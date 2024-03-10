#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // **************** Your code here ****************
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";
  TCPSocket tcp_sock;   // 建立 TCP 连接客户端
  tcp_sock.connect( Address( host, "http" ) );   // 客户端直接连接服务器
  // 写入字节流中，http 规范字符串
  tcp_sock.write( "GET " + path + " " + "HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n" );
  tcp_sock.shutdown(SHUT_WR);   // 关闭写入端
  while ( !tcp_sock.eof() ) {   // 没到结尾，就一直读取
    string buffer;
    tcp_sock.read( buffer );
    if ( buffer.size() )
      cout << buffer;
  }
  tcp_sock.close();
  // **************** Your code here ****************
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
