#include <iostream>

#include "ethernet_frame.hh"
#include "exception.hh"
#include "helpers.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  const uint32_t next_ip = next_hop.ipv4_numeric();
  if ( arp_cache_.contains( next_ip ) ) {
    if ( timer_ < arp_cache_[next_ip].second ) {
      transmit( {
        { arp_cache_[next_ip].first, ethernet_address_, EthernetHeader::TYPE_IPv4 },
        serialize( dgram ),
      } );
      return;
    }
    arp_cache_.erase( next_ip );
  }

  pending_dgrams_[next_ip].push( { dgram, timer_ + APR_REQUEST_EXPIRE_TIME } );
  if ( timer_ < arp_expire_time_[next_ip] )
    return;
  arp_expire_time_[next_ip] = timer_ + APR_REQUEST_EXPIRE_TIME;
  transmit( {
    { ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP },
    serialize( make_arp_msg( ARPMessage::OPCODE_REQUEST, {}, next_ip ) ),
  } );
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( EthernetFrame frame )
{
  const EthernetHeader header = frame.header;
  if ( header.dst != ethernet_address_ && header.dst != ETHERNET_BROADCAST )
    return;

  if ( header.type == EthernetHeader::TYPE_IPv4 ) {
    InternetDatagram dgram;
    if ( parse( dgram, frame.payload ) )
      datagrams_received_.emplace( dgram );
    return;
  }

  ARPMessage arp_msg;
  if ( !parse( arp_msg, frame.payload ) )
    return;
  const uint32_t sender_ip = arp_msg.sender_ip_address;
  const uint32_t target_ip = arp_msg.target_ip_address;
  arp_cache_[sender_ip] = { header.src, timer_ + IP_ETHERNET_MAPPING_DURATION };

  if ( arp_msg.opcode == ARPMessage::OPCODE_REPLY ) {
    auto& datagrams = pending_dgrams_[sender_ip];
    while ( !datagrams.empty() ) {
      if ( timer_ < datagrams.front().second )
        transmit(
          { { header.src, ethernet_address_, EthernetHeader::TYPE_IPv4 }, serialize( datagrams.front().first ) } );
      datagrams.pop();
    }
    pending_dgrams_.erase( sender_ip );
  } else if ( target_ip == ip_address_.ipv4_numeric() ) {
    const ARPMessage reply = make_arp_msg( ARPMessage::OPCODE_REPLY, arp_msg.sender_ethernet_address, sender_ip );
    transmit( {
      { reply.target_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP },
      serialize( reply ),
    } );
  }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  timer_ += ms_since_last_tick;
}

ARPMessage NetworkInterface::make_arp_msg( uint16_t opcode, EthernetAddress target_ether_addr, uint32_t target_ip )
{
  return { .opcode = opcode,
           .sender_ethernet_address = ethernet_address_,
           .sender_ip_address = ip_address_.ipv4_numeric(),
           .target_ethernet_address = target_ether_addr,
           .target_ip_address = target_ip };
}