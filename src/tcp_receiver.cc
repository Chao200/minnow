#include "tcp_receiver.hh"

using namespace std;

// 没有经过初始化的 ISN 为 0
TCPReceiver::TCPReceiver() : zero_point_( 0 ), init_( false ) {}

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if ( message.SYN ) {
    init_ = true;
    zero_point_ = message.seqno;
    // SYN 是不入 stream 的，不需要 insert
    message.seqno = message.seqno + 1; // SYN 算 seqno，故+1
  }

  if ( init_ ) { // 初始化之后
    // 需要将 seqno(32)->absolute_seqno(64)->stream_index(64)
    // 所以需要 absolute_seqno-1 = stream_index
    // inbound_stream.bytes_pushed() 没有 0，比 stream_index 多 1
    uint64_t first_index = message.seqno.unwrap( zero_point_, inbound_stream.bytes_pushed() ) - 1;
    reassembler.insert( first_index, message.payload, message.FIN, inbound_stream );
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // 由于 window_size 是 16 位，所以最多只能是 16 位
  uint16_t window_size = inbound_stream.available_capacity() > 0xffff
                           ? 0xffff
                           : static_cast<uint16_t>( inbound_stream.available_capacity() );

  if ( !init_ ) // 如果没有初始化 ISN，初始化 TCPReceiverMessage 的方式
    return TCPReceiverMessage { nullopt, window_size };

  /* 此时需要回送 ackno
    stream_index->absolute_seqno->seq_no->ackno
    inbound_stream.bytes_pushed() -1 为 stream_index
    stream_index + 1 为 absolute_seqno
    ackno = seq_no + 1
  */
  Wrap32 ackno = Wrap32::wrap( inbound_stream.bytes_pushed(), zero_point_ ) + 1;

  // FIN 也要 ack
  if ( inbound_stream.is_closed() )
    ackno = ackno + 1;

  return TCPReceiverMessage { ackno, window_size };
}
