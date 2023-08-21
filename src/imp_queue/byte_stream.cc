#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : 
  capacity_( capacity ), buffer_(), peek_(), 
  bytes_pushed_(0), bytes_poped_(0), 
  closed_(false), error_(false)
   {}

void Writer::push( string data )
{
  if (closed_ || error_) return;
  uint64_t push_size  = min(data.size(), available_capacity());
  bytes_pushed_ += push_size;
  for (uint64_t i = 0; i < push_size; ++i)
  {
    if (buffer_.empty())
    {
      peek_.clear();
      peek_ += data[i];
    }
    buffer_.push(data[i]);
  }
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
  // string s = "";
  // s += buffer_.front();
  // return s;
  return peek_;
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
  if (buffer_.size() < len) 
  {
    error_ = true;
    return;
  }
  bytes_poped_ += len;
  for (uint64_t i = 0; i < len; ++i)
  {
    buffer_.pop();
  }
  peek_.clear();
  if (!buffer_.empty()) peek_ += buffer_.front();
}

uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_poped_;
}
