#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) :
  read_bytes(0), write_bytes(0), closed(false), finished(false),
  stream(deque<char>{}), capacity_( capacity ) {}

bool Writer::is_closed() const
{
  // Your code here.
  return closed;
}

void Writer::push( string data )
{
  // Your code here.
  uint64_t len = min(
    available_capacity(),
    data.size()
  );
  write_bytes += len;
  for (uint64_t i = 0; i < len; i++)
    stream.push_back(data[i]);
}

void Writer::close()
{
  // Your code here.
  closed = true;
  if (stream.empty())
    finished = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - stream.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return write_bytes;
}

bool Reader::is_finished() const
{
  // Your code here.
  return finished;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return read_bytes;
}

string_view Reader::peek() const
{
  // Your code here.
  return string_view(&stream.front(), 1);
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  len = min(len, stream.size());
  read_bytes += len;
  while (len--) {
    stream.pop_front();
  }

  if (closed && stream.empty())
    finished = true;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return stream.size();
}
