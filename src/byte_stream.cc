#include "byte_stream.hh"
#include <iostream>
using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{  
  for (auto byte: data) {
    if (available_capacity() > 0) {
      stream.push_back(byte);
      pushed_++;
    } else {
      break;
    }
  }
  peek_ = string{stream.front()};
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - stream.size();
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_;
}

string_view Reader::peek() const
{
  return string_view{peek_};
}

void Reader::pop( uint64_t len )
{
  len = min(len, stream.size());
  popped_ += len;
  while (len-- > 0) {
    stream.pop_front();
  }
  if (stream.size() > 0) {
    peek_ = string{stream.front()};
  } else {
    peek_ = "";
  }
}

bool Reader::is_finished() const
{
  return popped_ == pushed_ && closed_;
}

uint64_t Reader::bytes_buffered() const
{
  return stream.size();
}

uint64_t Reader::bytes_popped() const
{
  return popped_;
}

