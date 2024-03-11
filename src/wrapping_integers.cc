#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // 直接对 64 位的 n 阶段就可以得到对应的 mod 2^32 值
  return Wrap32 { ( zero_point.raw_value_ + static_cast<uint32_t>( n ) ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint32_t offset = raw_value_ - zero_point.raw_value_;   // 得到 seqno 的偏移量
  uint32_t check32 = static_cast<uint32_t>( checkpoint );    // 低 32 位
  uint64_t check64 = ( checkpoint & 0xffffffff00000000 );    // 高 32 位

  // 刚好相等
  if ( check32 == offset )
    return check64 + offset;
  // 不相等的时候，offset 多走了几圈，可能在 check32 左右两边
  // case1. offset 到 check32 距离小于 check32 到 offset 距离
  if ( ( offset - check32 ) < ( check32 - offset ) )
    return offset > check32 ? 
            check64 + offset : check64 + offset + 0x100000000;
  // case2. offset 到 check32 距离大于 check32 到 offset 距离
  else
    return offset > check32 ? 
              ( ( check64 == 0 ) ? offset : ( check64 + offset - 0x100000000 ) ) 
              : check64 + offset;
}
