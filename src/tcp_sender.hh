#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <list>
 
class TCPSender
{
  // *********************************
  // ************ 报文段相关 ************
  Wrap32 isn_;   // 初始化 ISN
  bool is_SYN_ = false;  // 是否为 SYN
  bool is_FIN_ = false;  // 是否为 FIN
  uint64_t abs_seqno_ = 0; // 记录 push 时，每一段的abs seqno，要用绝对序列号，因为相对序列号会溢出
  std::list<TCPSenderMessage> readied_segments_ = {};        // 记录所有准备好发送的段

  // *********************************
  // ************ 超时相关 ************
  bool is_start_RTO_timer_ = false;   // 是否启动 RTO 计时器
  uint64_t initial_RTO_ms_;   // 默认初始 RTO 时间，单位 ms
  int RTO_ms_;  // RTO_ms_ 可能为负数，所以不可以是 uint64_t 或者 size_t

  // *********************************
  // ************ 重传相关 ************
  std::list<TCPSenderMessage> outstanding_segments_ = {};    // 记录所有已发送未 ACK 的段
  uint64_t outstanding_bytes_nums_ = 0;                       // 记录已发送未 ACK 的字节数
  uint64_t consecutive_retransmissions_nums_ = 0;             // 连续重传次数
  
  // *********************************
  // *********** 接收端信息 ************
  TCPReceiverMessage rcv_msg_ = {}; // receiver 返回给 sender 的消息，即 ack 和 window_size
  uint16_t window_size_ = 1;   // 记录接收方的窗口大小，初始为 1

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
