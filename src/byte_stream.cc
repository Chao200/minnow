#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

// *************** Your code start here ***************

ByteStream::ByteStream( uint64_t capacity )  // 字节流 class 的构造函数
  : capacity_( capacity ), buffer_(), bytes_pushed_( 0 ), 
    bytes_poped_( 0 ), closed_( false ), error_( false ) {}

// *************** Writer ***************
/*
* @brief 将 data 写入字节流中
* @param data 待写入字节流的数据
*/
void Writer::push( string data )
{
  // 得到可以写入字节流的字节数
  uint64_t push_size = min( data.size(), available_capacity() );
  buffer_ += data.substr( 0, push_size );  // 写入 buffer
  bytes_pushed_ += push_size;   // 增加已写入字节数
}

/*
* @brief signal 关闭字节流的写入端
*/
void Writer::close()
{
  closed_ = true;
}

/*
* @brief signal 设置字节流遇到了一个 error
*/
void Writer::set_error()
{
  error_ = true;
}

/*
* @brief 是否关闭了字节流的写入端
* @return bool 1 关 0 开
*/
bool Writer::is_closed() const
{
  return closed_;
}

/*
* @brief 返回可用的空间大小
* @return uint64_t 空间大小
*/
uint64_t Writer::available_capacity() const
{
  return capacity_ - buffer_.size();
}

/*
* @brief 返回累计写入字节流的数量
* @return uint64_t 累计字节流数量
*/
uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}


// *************** Reader ***************
/*
* @brief 返回累计写入字节流的数量
* @return uint64_t 累计字节流数量
*/
string_view Reader::peek() const
{
  return buffer_;
}

/*
* @brief 字节流是否结束，即 ① buffer 为空；② 写入端关闭
* @return bool 1 结束
*/
bool Reader::is_finished() const
{
  return buffer_.empty() && closed_;
}

/*
* @brief 字节流是否出现 error
* @return bool 1 出现 error
*/
bool Reader::has_error() const
{
  return error_;
}

/*
* @brief 从字节流中移除 len 长度字节
* @param len 字节大小
*/
void Reader::pop( uint64_t len )
{
  // buffer 内容和 len 取 min
  uint64_t pop_size = min( len, buffer_.size() );
  buffer_ = buffer_.substr( pop_size );  // 截取移除后的内容
  bytes_poped_ += pop_size;   // 增加移除字节数
}

/*
* @brief buffer 中现存的字节数
* @return uint64_t 字节数
*/
uint64_t Reader::bytes_buffered() const
{
  return buffer_.size();
}

/*
* @brief 从字节流中累计弹出的字节数
* @return uint64_t 字节数
*/
uint64_t Reader::bytes_popped() const
{
  return bytes_poped_;
}

// *************** Your code end here ***************