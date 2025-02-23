#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  const uint64_t byte_popped = reader().bytes_popped();
  return sent_ackno_.unwrap( isn_, byte_popped ) - received_ackno_.unwrap( isn_, byte_popped );
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retrans_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  const bool is_SYN = ( sent_ackno_ == isn_ );
  const bool stream_closed = writer().is_closed();
  if ( fin_ ) return;

  string payload;
  while ( is_SYN || ( winsize_ != 0 && ( stream_closed || reader().bytes_buffered() > 0 ) ) ) {
    uint64_t payload_size = min(winsize_, TCPConfig::MAX_PAYLOAD_SIZE);
    if ( is_SYN && winsize_ != 0 )
      payload_size -= 1;
    read( reader(), payload_size, payload );
    winsize_ -= payload.size();
    TCPSenderMessage msg = TCPSenderMessage {
      sent_ackno_,
      is_SYN,
      payload,
      stream_closed && winsize_ != 0 && reader().bytes_buffered() == 0,
      reader().has_error()
    };
    winsize_ -= msg.FIN;
    fin_ = msg.FIN;
    sent_ackno_ = sent_ackno_ + msg.sequence_length();
    transmit( msg );
    if ( in_flight_msg_.empty() )
      expire_time_ = timer_ + rto_ms_;
    in_flight_msg_.push( msg );
    if ( is_SYN || msg.FIN )
      break;
  }

  if ( winsize_ == 0 && ( reader().bytes_buffered() > 0 || stream_closed ) && in_flight_msg_.empty() ) {
    TCPSenderMessage msg = make_empty_message();
    msg.FIN = stream_closed && reader().bytes_buffered() == 0;
    read( reader(), !msg.FIN, msg.payload );
    provoking_seqno_ = msg.seqno;
    transmit ( msg );
    in_flight_msg_.push( msg );
    sent_ackno_ = sent_ackno_ + 1;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return TCPSenderMessage {
    .seqno = sent_ackno_,
    .RST = reader().has_error()
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  const uint64_t byte_popped = reader().bytes_popped();
  if ( !msg.ackno.has_value() ) {
    if ( msg.window_size == 0 ) reader().set_error();
    if ( byte_popped == 0 ) winsize_ = msg.window_size;
    return;
  }
  if ( provoking_seqno_ == msg.ackno )
    provoking_seqno_.reset();

  const uint64_t msg_abs_ackno = msg.ackno->unwrap( isn_, byte_popped );
  const uint64_t sent_abs_ackno = sent_ackno_.unwrap( isn_, byte_popped );
  const uint64_t received_abs_ackno = received_ackno_.unwrap( isn_, byte_popped );
  
  if ( msg_abs_ackno > sent_abs_ackno ) return;
  if ( msg_abs_ackno > received_abs_ackno && !in_flight_msg_.empty() ) {
    rto_ms_ = initial_RTO_ms_;
    retrans_cnt_ = 0;
    received_ackno_ = *msg.ackno;

    while (
      !in_flight_msg_.empty() &&
      msg_abs_ackno >= ( 
        in_flight_msg_.front().seqno.unwrap( isn_, byte_popped ) + 
        in_flight_msg_.front().sequence_length() 
      )
    ) {
      in_flight_msg_.pop();
    }
    if ( !in_flight_msg_.empty() )
      expire_time_ = timer_ + rto_ms_;
  }
  if ( msg_abs_ackno + msg.window_size > sent_abs_ackno + winsize_ ) {
    winsize_ = msg.window_size - ( sent_abs_ackno - msg_abs_ackno );
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  timer_ += ms_since_last_tick;
  if ( !in_flight_msg_.empty() && timer_ >= expire_time_ ) {
    if ( !provoking_seqno_.has_value() )
      rto_ms_ <<= 1;
    expire_time_ += rto_ms_;
    retrans_cnt_ += 1;
    transmit( in_flight_msg_.front() );
  }
}
