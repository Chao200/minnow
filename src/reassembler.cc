#include "reassembler.hh"

using namespace std;

Reassembler::Segment::Segment( uint64_t first, std::string data, bool eof )
  : first_( first ), last_( first + data.size() ), data_( data ), eof_( eof )
{}

Reassembler::Segment::Segment( uint64_t first, uint64_t last, std::string data, bool eof )
  : first_( first ), last_( last ), data_( data ), eof_( eof )
{}

Reassembler::Reassembler() : buffer_() {}

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
  // 记录是否 push 了当前的数据
  bool pushed = false;
  // 当前要加入的 data 和 buffer 中的 data 比较
  for ( const Segment& seg : buffer_ ) {
    // buffer 中的在 data 之前
    if ( pushed || seg.last_ < first_index ) {
      temp.push_back( seg );
      continue;
    }

    // buffer 中的在 data 之后
    if ( seg.first_ > last_index ) {
      temp.push_back( Segment( first_index, last_index, data, is_last_substring ) );
      pushed = true;
      temp.push_back( seg );
      continue;
    }

    // // 方法 1
    // if (first_index < seg.first_ && last_index > seg.last_)
    // {
    //   data = data;
    // }
    // else if (first_index >= seg.first_ && last_index <= seg.last_)
    // {
    //   data = seg.data_;
    //   first_index = seg.first_;
    //   last_index = seg.last_;
    //   is_last_substring = seg.eof_;
    // }
    // else if (first_index < seg.first_)
    // {
    //   data += seg.data_.substr(last_index-seg.first_);
    //   last_index = seg.last_;
    //   is_last_substring = seg.eof_;
    // }
    // else if (last_index > seg.last_)
    // {
    //   data = seg.data_.substr(0, first_index-seg.first_) + data;
    //   first_index = seg.first_;
    // }

    // if (is_last_substring) break;

    // 方法 2
    // data 在 buffer 范围内有数据，但是可能有一侧越界
    if ( seg.first_ < first_index ) {
      // 一定要 + data，而不是 data +=，顺序问题
      data = seg.data_.substr( 0, first_index - seg.first_ ) + data;
      first_index = seg.first_;
    }

    if ( is_last_substring )
      break;

    if ( seg.last_ > last_index ) {
      data += seg.data_.substr( last_index - seg.first_ );
      last_index = seg.last_;
      is_last_substring = seg.eof_;
    }
  }

  if ( !pushed ) {
    temp.push_back( Segment( first_index, last_index, data, is_last_substring ) );
  }
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

uint64_t Reassembler::bytes_pending() const
{
  uint64_t bytes_count = 0;
  for ( const Segment& s : buffer_ ) {
    bytes_count += s.data_.size();
  }
  return bytes_count;
}
