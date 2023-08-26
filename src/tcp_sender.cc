#include "tcp_sender.hh"
#include "tcp_config.hh"
 
#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , RTO_ms_( initial_RTO_ms )
{
  // 在 receiver 给 sender 发送 TCPReceiverMessage 之前
  // 需要初始化 ack 和 window_size
  rcv_msg_.ackno = isn_;
  rcv_msg_.window_size = 1;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_bytes_nums_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_nums_;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // 发送 TCPSenderMessage 给 receiver
  if (readied_segments_.empty()) return nullopt;
  // 非空返回首个报文段
  auto snd_msg = readied_segments_.front();
  readied_segments_.pop_front();
  // 并启动计时器
  is_start_RTO_timer_ = true;

  return snd_msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  /*
    从字节流中读取数据，将字节流打包，也就是组装到 TCPSenderMessage
    包括 SYN FIN Payload seqno
  */
  // 当已发送但未 ACK 的字节数没有超过接收方的 window_size 时，继续读取字节流
  // 否则，暂不读取
  while (outstanding_bytes_nums_ < rcv_msg_.window_size)
  {
    if (is_FIN_) break;

    // 将 SYN FIN Payload seqno 打包到 snd_msg 中
    TCPSenderMessage snd_msg;
    
    // 1. 是否为 SYN，不是的话进行初始化
    if (is_SYN_ == false)
    {
      is_SYN_ = true;
      snd_msg.SYN = true;
      snd_msg.seqno = isn_;   // 为 ISN
    }
    else
    {
      // 如果不是 SYN，得到 seqno
      snd_msg.seqno = Wrap32::wrap(abs_seqno_, isn_);
    }

    // 求可以打包的数据最小 size
    // ① TCPConfig::MAX_PAYLOAD_SIZE
    // ② 接收方的窗口大小减去已发送但未 ACK 的字节数，注意不单单是接收方的窗口大小
    // 还有数据在发送但未 ACK，所以必须减去 outstanding_bytes_nums_
    // ③ stream 中缓存的字节数
    auto size = min
                (
                  min(
                        TCPConfig::MAX_PAYLOAD_SIZE, 
                        rcv_msg_.window_size - outstanding_bytes_nums_
                      ),
                  outbound_stream.bytes_buffered()
                );
    
    // 从 outbound_stream 中读取 size 大小字节到 snd_msg 的 Payload
    read(outbound_stream, size, snd_msg.payload);

    // 前面已经得到了 SYN、seqno、Payload，还剩下 FIN
    // stream 已经结束并且 
    // outstanding_bytes_nums_ 加上刚刚打包的 snd_msg 小于接收方窗口大小
    // 因为 FIN 还要占一个字节，但是此时 FIN 默认是 false，还没加上
    // 所以只能是小于，不可以等于
    if (outbound_stream.is_finished() && 
          outstanding_bytes_nums_
            + snd_msg.sequence_length() 
              < rcv_msg_.window_size
       )
    {
      // if (!is_FIN_)
      // {  // 标记 FIN
        is_FIN_ = true;
        snd_msg.FIN = true;
      // }
    }

    // 如果 snd_msg 为空，当然 seqno 是有效信息，break
    if (snd_msg.sequence_length()==0) break;

    /*  snd_msg 不为空则执行下面操作  */
    // 加入到 readied_segments_ 和 outstanding_segments_
    readied_segments_.push_back(snd_msg);
    outstanding_segments_.push_back(snd_msg);
    // 增加 outstanding_bytes_nums_
    outstanding_bytes_nums_ += snd_msg.sequence_length();
    // 增加 abs_seqno_
    abs_seqno_ += snd_msg.sequence_length();
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // 发送 empty 报文段
  TCPSenderMessage snd_msg;
  // 只要 seqno 信息即可，其余为 0 或空
  snd_msg.seqno = Wrap32::wrap(abs_seqno_, isn_);
  return snd_msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // 得到 TCPReceiverMessage 信息，回送给 sender
  // 包括 ackno 和 window_size
  rcv_msg_ = msg;
  if ( msg.window_size == 0 )  // 如果接收方窗口大小为 0，则设置为 1
    rcv_msg_.window_size = 1;
  
  // 记录接收方的窗口大小
  window_size_ = msg.window_size;
  // 如果 ackno 有值
  if (msg.ackno.has_value())
  {
    // ackno 转化为 abs_seqno，如果大于发送方的 abs_seqno_，则 return
    // abs_seqno_ 是发送方下一个发送的报文段序号，超过这个序号肯定不对，直接不进行操作 return
    if ( msg.ackno.value().unwrap( isn_, abs_seqno_ ) > abs_seqno_ )
      return;

    // 缓存的报文段个数不为 0，并且缓存中的首个报文段 abs_seqno+报文段长度后小于接收方传回来的 abs_seqno
    // 那么就可以移动发送方的窗口，并删除缓存
    while (outstanding_bytes_nums_!=0 && 
            outstanding_segments_.front().seqno.unwrap(isn_, abs_seqno_)
            + outstanding_segments_.front().sequence_length()
              <= msg.ackno.value().unwrap(isn_, abs_seqno_)
          )
    {
      // 减少 outstanding_bytes_nums_
      outstanding_bytes_nums_ -= outstanding_segments_.front().sequence_length();
      // 删除
      outstanding_segments_.pop_front();
      // 如果没有缓存了，终止计时器
      if (outstanding_bytes_nums_ == 0) is_start_RTO_timer_ = false;
      // else is_start_RTO_timer_ = true;
      // 重置重传次数和 RTO
      consecutive_retransmissions_nums_ = 0;
      RTO_ms_ = initial_RTO_ms_;
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // 如果启动了计时器
  if (is_start_RTO_timer_) 
  {
    // 用 RTO_ms -= 距离上次调用该函数的时间
    RTO_ms_ -= ms_since_last_tick;

    // 如果非正，则超时
    if (RTO_ms_ <= 0)
    {
      // 马上重传，将第一个加入缓存中的报文段，添加到 readied_segments_ 的首位
      readied_segments_.push_front(outstanding_segments_.front());
      // 重传计数
      ++consecutive_retransmissions_nums_;

      // 如果接收窗口大于 0，就加倍 RTO
      if (window_size_ > 0) 
        // RTO_ms_ = pow(2, consecutive_retransmissions_nums_) * initial_RTO_ms_;
        RTO_ms_ = initial_RTO_ms_ << consecutive_retransmissions_nums_;
      // 接收窗口为 0 的时候，不加倍 RTO
      else RTO_ms_ = initial_RTO_ms_;
    }
  }
}