#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  const uint64_t mod = 1ul << 32;
  uint64_t target = (raw_value_ + mod - zero_point.raw_value_) % mod;
  if (target + (mod >> 1) < checkpoint)
    target += ((checkpoint - target + (mod >> 1)) / mod) * mod;
  return target;
}
