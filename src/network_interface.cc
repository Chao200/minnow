#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address )
    , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // 映射中能找到
  if (mapping_.contains(next_hop.ipv4_numeric()))
  {
    // 构建链路层帧
    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_IPv4;
    frame.header.src = ethernet_address_;
    frame.header.dst = mapping_[next_hop.ipv4_numeric()].first;
    // 装帧
    frame.payload = serialize(dgram);
    // 入发送队列
    send_queue_.push(frame);
  }
  else
  {
    // 如果 mapping 找不到，就需要发送 ARP 请求
    // 如果之前发送了 ARP 请求还存在，即没超过 5s，就 return
    if (arp_request_timer_.contains(next_hop.ipv4_numeric())) return;

    // 构建 ARP 请求
    ARPMessage arp_msg;
    arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
    arp_msg.sender_ethernet_address = ethernet_address_;
    arp_msg.sender_ip_address = ip_address_.ipv4_numeric();
    // 不需要填写 target_ethernet_address
    arp_msg.target_ip_address = next_hop.ipv4_numeric();

    EthernetFrame frame;
    frame.header.type = EthernetHeader::TYPE_ARP;
    // 源链路层地址
    frame.header.src = ethernet_address_;
    // 目的链路层地址，即广播地址
    frame.header.dst = ETHERNET_BROADCAST;
    // 将 ARP 请求作为有效载荷
    frame.payload = serialize(arp_msg);
    // 入发送队列
    send_queue_.push(frame);
    // 记录这个 ARP 请求
    arp_request_timer_.insert({next_hop.ipv4_numeric(), 0});
    // 记录这个 ARP 请求待发送的数据，入队
    // 后面等 ARP 响应到达的时候，就可以将存储的数据发送过去
    nexthop_ipdatagram_mapping_[next_hop.ipv4_numeric()].push(dgram);
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // 如果该帧中目的地址不是 广播地址 或者 当前链路层地址，就 return
  if (frame.header.dst != ethernet_address_ &&
      frame.header.dst != ETHERNET_BROADCAST) return nullopt;
  
  // 如果该帧时 IPv4 类型，即正常数据报，解帧成包
  if (frame.header.type == EthernetHeader::TYPE_IPv4)
  {
    InternetDatagram datagram;
    if (parse( datagram, frame.payload )) return datagram;
    else return nullopt;
  }

  // 如果该帧是 ARP 类型
  if (frame.header.type == EthernetHeader::TYPE_ARP)
  {
    ARPMessage arp_msg;
    if (!parse( arp_msg, frame.payload )) return nullopt;
    
    // 成功 parse 后加入到映射中
    mapping_.insert({arp_msg.sender_ip_address, 
                      {arp_msg.sender_ethernet_address, 0}});

    // 解析后得到的 ARP 信息中，是 ARP 请求信息，并且目的 IP 就是当前的 IP 地址
    if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST &&
          arp_msg.target_ip_address == ip_address_.ipv4_numeric())
    {
      EthernetFrame snd_frame;
      ARPMessage snd_arpmsg;
      snd_arpmsg.opcode = ARPMessage::OPCODE_REPLY;
      snd_arpmsg.sender_ip_address = ip_address_.ipv4_numeric();
      snd_arpmsg.sender_ethernet_address = ethernet_address_;
      snd_arpmsg.target_ip_address = arp_msg.sender_ip_address;
      snd_arpmsg.target_ethernet_address = arp_msg.sender_ethernet_address;

      snd_frame.header.dst = arp_msg.sender_ethernet_address;
      snd_frame.header.src = ethernet_address_;
      snd_frame.header.type = EthernetHeader::TYPE_ARP;
      snd_frame.payload = serialize(snd_arpmsg);
      send_queue_.push(snd_frame);
    }

    // ARP 响应
    else if (arp_msg.opcode == ARPMessage::OPCODE_REPLY)
    {
      // mapping_.insert({arp_msg.sender_ip_address, 
      //                 {arp_msg.sender_ethernet_address, 0}});
      // 得到该 IP 地址之前待发送的数据，并发送
      auto& ip_datagram = nexthop_ipdatagram_mapping_[arp_msg.sender_ip_address];
      while (!ip_datagram.empty())
      {
        send_datagram(ip_datagram.front(), Address::from_ipv4_numeric(arp_msg.sender_ip_address));
        ip_datagram.pop();
      }
      // 移除该 IP 地址
      nexthop_ipdatagram_mapping_.erase(arp_msg.sender_ip_address);
    }
  }
  return nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // 遍历 mapping 30s
  for (auto it = mapping_.begin(); it!=mapping_.end(); )
  {
    it->second.second += ms_since_last_tick;    // 更新时间
    if (it->second.second >= arp_mapping_timer_)  // 超时
      it = mapping_.erase(it);    // 删除该映射，返回值为 it 的下一个位置
    else ++it;
  }

  // 同理遍历 ARP 请求 5s
  for (auto it = arp_request_timer_.begin(); it!=arp_request_timer_.end();)
  {
    it->second += ms_since_last_tick;
    if (it->second >= arp_send_timer_)
      it = arp_request_timer_.erase(it);
    else ++it;
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  // 弹出首元
  if (send_queue_.empty()) return nullopt;
  auto snd_msg = send_queue_.front();
  send_queue_.pop();
  return snd_msg;
}
