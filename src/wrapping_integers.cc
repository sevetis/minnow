#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  const uint64_t bin_32 = static_cast<uint64_t>(UINT32_MAX) + 1;
  uint64_t cur_point = (raw_value_ - zero_point.raw_value_ + bin_32) % bin_32;
  if (cur_point < checkpoint) {
    cur_point += bin_32 * ((checkpoint - cur_point) / bin_32);
    if (checkpoint - cur_point > bin_32 / 2)
      cur_point += bin_32;
  }
  return cur_point;
}
