#include "tcp_receiver.hh"

using namespace std;

TCPReceiver::TCPReceiver()
: zero_point_(0), init_(false)
{}

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if (message.SYN)
  {
    init_ = true;
    zero_point_ = message.seqno;
    // SYN 是不入 stream 的，所以 +1
    message.seqno = message.seqno + 1;
  }

  if (init_)
  {
    // 将 seqno->absolute_seqno->stream_index
    // 所以需要 -1
    uint64_t first_index = message.seqno.unwrap(zero_point_, inbound_stream.bytes_pushed())-1;
    reassembler.insert(first_index, message.payload, message.FIN, inbound_stream);
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{ 
  uint16_t window_size = 
    inbound_stream.available_capacity() > 0xffff ? 
      0xffff : uint16_t(inbound_stream.available_capacity());

  if (!init_) return TCPReceiverMessage {nullopt, window_size};

  // stream_index->absolute_seqno->seq_no->ackno
  Wrap32 ackno = Wrap32::wrap(inbound_stream.bytes_pushed(), zero_point_)+1;
  
  if (inbound_stream.is_closed()) ackno = ackno + 1;
  
  return TCPReceiverMessage {ackno, window_size};
}
