#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length_: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length_,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length_ ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  route_table_.push_back( RouteTable_( route_prefix, prefix_length_, next_hop, interface_num ) );
}

void Router::route()
{
  for ( auto& cur_interface : interfaces_ ) {
    std::optional<InternetDatagram> datagram = cur_interface.maybe_receive();
    while ( datagram.has_value() && datagram->header.ttl > 1 ) {
      datagram->header.ttl--;
      datagram->header.compute_checksum();
      // 最长前缀匹配，需要目的地的 IP 地址和路由表中的匹配
      uint32_t dst = datagram->header.dst;
      auto match = route_table_.end();
      // std::optional<uint8_t> max_prefix_length {};
      uint8_t max_prefix_length = route_table_.begin()->prefix_length_;
      for ( auto it = route_table_.begin(); it != route_table_.end(); ++it ) {
        if ( it->prefix_length_ == 0
             || ( dst >> ( 32 - it->prefix_length_ ) )
                  == ( ( it->route_prefix_ ) >> ( 32 - it->prefix_length_ ) ) ) {
          if ( max_prefix_length <= it->prefix_length_ ) {
            match = it;
            max_prefix_length = it->prefix_length_;
          }
          // if ( !max_prefix_length.has_value() || *max_prefix_length < it->prefix_length_ )
          // {
          //   max_prefix_length = it->prefix_length_;
          //   match = it;
          // }
        }
      }
      if ( match != route_table_.end() ) {
        if ( match->next_hop_ )
          interface( match->interface_num_ ).send_datagram( *datagram, *( match->next_hop_ ) );
        else
          interface( match->interface_num_ ).send_datagram( *datagram, Address::from_ipv4_numeric( dst ) );
        // interface(match->interface_num_).send_datagram(*datagram, match->next_hop_.value_or(
        // Address::from_ipv4_numeric( dst )));
      }

      datagram = cur_interface.maybe_receive();
    }
  }
}