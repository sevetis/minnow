#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (!FIN)
    FIN = message.FIN;
  if (message.RST) {
    reassembler_.reader().set_error();
    return;
  }
  if (message.SYN) {
    SYN = message.SYN;
    zero_point = message.seqno;
  }

  if (zero_point.has_value()) {
    uint64_t f_idx_ = message.seqno.unwrap(zero_point.value(), received_());
    if (f_idx_ == 0 && message.SYN) {
      reassembler_.insert(0, message.payload, message.FIN);
    } else if (f_idx_ != 0) {
      reassembler_.insert(f_idx_ - 1, message.payload, message.FIN);
    }
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  auto ackno = zero_point;
  if (ackno.has_value())
    ackno = ackno.value() + received_();
  return {
    ackno,
    static_cast<uint16_t>(min(writer().available_capacity(), ((1ul << 16) - 1))),
    reassembler_.writer().has_error()
  };
}

uint64_t TCPReceiver::received_() const {
  return reassembler_.writer().bytes_pushed() + SYN + (reassembler_.writer().is_closed() && FIN);
}
