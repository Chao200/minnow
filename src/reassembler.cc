#include "reassembler.hh"

using namespace std;

Reassembler::Segment::Segment( uint64_t first, std::string data, bool eof )
  : first_( first ), last_( first + data.size() ), data_( data ), eof_( eof )
{}

Reassembler::Segment::Segment( uint64_t first, uint64_t last, std::string data, bool eof )
  : first_( first ), last_( last ), data_( data ), eof_( eof )
{}

/*
* @brief buffer_ 存放已经到来还没 push 的数据
*/
Reassembler::Reassembler() : buffer_() {}

/*
* @brief 将数据插入到 buffer 中
* @param first_index 待插入数据的首部位置
* @param data 待插入数据
* @param is_last_substring 是否是最后一个字符串，即 eof
* @param output 插入到 Writer 中
*/
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Writer 的 capacity 范围
  uint64_t min_index = output.bytes_pushed();
  uint64_t max_index = min_index + output.available_capacity();
  // data 的最后一个索引
  uint64_t last_index = first_index + data.size();

  // case1: data 在 capacity 之外
  if ( first_index > max_index || last_index < min_index )
    return;

  // case2: data 必然有存在于 capacity 之内的，但是一端索引可能超过范围
  // 如果 last_index 超过
  if ( last_index > max_index ) {
    data = data.substr( 0, max_index - first_index );
    last_index = max_index;
  }

  // 如果 first_index 超过
  if ( first_index < min_index ) {
    data = data.substr( min_index - first_index );
    first_index = min_index;
  }

  // 这样得到的 data 和索引都是属于 capacity 范围之内的

  std::list<Segment> temp;
  // 记录是否 push 了当前的数据，默认为 false
  bool pushed = false;
  // 当前要加入的 data 和 buffer 中的 data 比较，即调整位置，使得索引最小的在最前面
  for ( const Segment& seg : buffer_ ) {
    // case1: buffer 中的数据在 data 之前，或者已经 push 过了
    if ( pushed || seg.last_ < first_index ) {
      temp.push_back( seg );
      continue;
    }

    // case2: buffer 中的数据在 data 之后
    if ( seg.first_ > last_index ) {
      temp.push_back( Segment( first_index, last_index, data, is_last_substring ) );
      pushed = true;
      temp.push_back( seg );
      continue;
    }

    // case3: data 与 buffer 中的数据有重叠
    if ( seg.first_ < first_index ) {
      // 一定要 + data，而不是 data +=，顺序问题
      data = seg.data_.substr( 0, first_index - seg.first_ ) + data;
      first_index = seg.first_;
    }

    if ( seg.last_ > last_index ) {
      data += seg.data_.substr( last_index - seg.first_ );
      last_index = seg.last_;
      is_last_substring = seg.eof_;
    }

    if ( is_last_substring )  // 是否 break
      break;
  }

  if ( !pushed ) {  // buffer 为空的时候
    temp.push_back( Segment( first_index, last_index, data, is_last_substring ) );
  }
  // 此时 temp 已经有序，需要 swap
  swap( buffer_, temp );

  // 从 buffer_ 中弹出首元，判断是否可以写入字节流
  const Segment& substring = buffer_.front();
  if ( substring.first_ == min_index ) {
    output.push( substring.data_ );
    if ( substring.eof_ )
      output.close();
    buffer_.pop_front();
  }
}

/*
* @brief 写入了多少字节
* @return uint64_t 字节数
*/
uint64_t Reassembler::bytes_pending() const
{
  uint64_t bytes_count = 0;
  for ( const Segment& s : buffer_ ) {
    bytes_count += s.data_.size();
  }
  return bytes_count;
}
