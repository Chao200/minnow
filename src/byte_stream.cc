#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity ), buffer_(), bytes_pushed_( 0 ), bytes_poped_( 0 ), closed_( false ), error_( false )
{}

void Writer::push( string data )
{
  uint64_t push_size = min( data.size(), available_capacity() );
  buffer_ += data.substr( 0, push_size );
  bytes_pushed_ += push_size;
}

void Writer::close()
{
  closed_ = true;
}

void Writer::set_error()
{
  error_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  return buffer_;
}

bool Reader::is_finished() const
{
  return buffer_.empty() && closed_;
}

bool Reader::has_error() const
{
  return error_;
}

void Reader::pop( uint64_t len )
{
  uint64_t pop_size = min( len, buffer_.size() );
  buffer_ = buffer_.substr( pop_size );
  bytes_poped_ += pop_size;
}

// std::string Reader::pop2( uint64_t len )
// {
//   uint64_t pop_size = min( len, buffer_.size() );
//   std::string res = buffer_.substr(0, pop_size);
//   buffer_ = buffer_.substr( pop_size );
//   bytes_poped_ += pop_size;
//   return res;
// }

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_poped_;
}
