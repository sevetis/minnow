#include "router.hh"

#include <iostream>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  uint32_t mask = prefix_length == 0 ? 0 : ~( ( 1 << ( 32 - prefix_length ) ) - 1 );
  route_table_[{ route_prefix, mask }] = { next_hop, interface_num };
}

// Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
void Router::route()
{
  for ( auto& interface_ : interfaces_ ) {
    auto& dgrams = interface_->datagrams_received();
    while ( !dgrams.empty() ) {
      auto dgram = std::move( dgrams.front() );
      dgrams.pop();

      if ( dgram.header.ttl <= 1 )
        continue;
      dgram.header.ttl--;
      dgram.header.compute_checksum();

      for ( const auto& [net, route] : route_table_ )
        if ( net.contain( dgram.header.dst ) ) {
          const auto& [next_hop, interface_num] = route;
          interface( interface_num )
            ->send_datagram( dgram, next_hop.value_or( Address::from_ipv4_numeric( dgram.header.dst ) ) );
          break;
        }
    }
  }
}
