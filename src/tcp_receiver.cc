#include "tcp_receiver.hh"
#include <cstdint>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reader().set_error();
    return;
  }
  if ( message.SYN ) {
    zero_ = message.seqno;
    ackno_ = *zero_ + 1;
  }
  if ( zero_ ) {
    const uint64_t read_index = writer().bytes_pushed();
    const uint64_t abs_seqno = message.seqno.unwrap( *zero_, read_index );
    const uint64_t stream_index = abs_seqno + message.SYN - 1;
    if ( read_index <= stream_index + message.payload.size() ) {
      reassembler_.insert( stream_index, message.payload, message.FIN );
      ackno_ = *ackno_ + ( writer().bytes_pushed() - read_index ) + writer().is_closed();
    }
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  return TCPReceiverMessage {
    ackno_,
    static_cast<uint16_t>( min( writer().available_capacity(), static_cast<uint64_t>( UINT16_MAX ) ) ),
    writer().has_error() };
}
