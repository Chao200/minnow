#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { ((zero_point.raw_value_ + uint32_t(n)) & 0xffffffff)};
  // return Wrap32 {uint32_t(zero_point.raw_value_ + uint32_t(n))};
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t abs_seqno = raw_value_ - zero_point.raw_value_;
  uint32_t check32 = uint32_t(checkpoint);
  uint64_t check64 = (checkpoint & 0xffffffff00000000);

  if (check32 == abs_seqno) return check64 + abs_seqno;
  if ((abs_seqno - check32) < (check32 - abs_seqno)) 
  {
    return abs_seqno > check32 ? 
      check64 + abs_seqno : check64 + abs_seqno + 0x100000000;
  }
  else return abs_seqno > check32 ? 
    (
      (check64 == 0) ? 
        abs_seqno : (check64 + abs_seqno - 0x100000000)
    ) :
    check64 + abs_seqno;
}
