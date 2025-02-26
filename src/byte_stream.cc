#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{  
  if ( is_closed() or available_capacity() == 0 or data.empty() )
    return;

  if ( data.length() > available_capacity() )
    data.resize( available_capacity() );

  pushed_ += data.length();
  buffered_ += data.length();

  stream_.emplace( std:: move( data ) );
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
  return capacity_ - buffered_;
}

uint64_t Writer::bytes_pushed() const
{
  return pushed_;
}

string_view Reader::peek() const
{
  return stream_.empty() ? string_view {} :
          string_view { stream_.front() }.substr( front_poped_ );
}

void Reader::pop( uint64_t len )
{
  if ( len > buffered_ )
    len = buffered_;
  
  popped_ += len;
  buffered_ -= len;

  while ( len > 0 ) {
    const uint64_t front_remain_ = stream_.front().length() - front_poped_;
    if ( len >= front_remain_ ) {
      front_poped_ = 0;
      len -= front_remain_;
      stream_.pop();
    } else {
      front_poped_ += len;
      break;
    }
  }

}

bool Reader::is_finished() const
{
  return popped_ == pushed_ && closed_;
}

uint64_t Reader::bytes_buffered() const
{
  return buffered_;
}

uint64_t Reader::bytes_popped() const
{
  return popped_;
}

