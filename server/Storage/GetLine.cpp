#include "GetLine.hpp"

#include <unistd.h>

#include "Log.hpp"

unsigned char GetLine::_buffer[GETLINE_BUFFER_SIZE];
unsigned char GetLine::publicBuffer[PUBLIC_BUFFER_SIZE];

void GetLine::initialize() {
  _fd          = -1;
  _readPos     = 0;
  _isReadError = false;
  clear();
}

std::string GetLine::nextLine() {
  vector::iterator  bi          = begin();
  vector::iterator  it          = bi + _readPos;
  vector::size_type before_size = size();
  for (; it != end(); ++it) {
    if (*it == '\n') {
      std::string line(bi + _readPos, it);
      _readPos += (it - bi) - _readPos + 1;
      return line;
    }
  }

  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "line", std::string(begin() + _readPos, end()));
  Log::log()(true, "before_size", before_size);
  Log::log()(true, "_readPos", _readPos);

  int read_size = read(_fd, _buffer, GETLINE_BUFFER_SIZE);
  if (read_size == -1) {
    _isReadError = true;
    Log::log()(LOG_LOCATION, strerror(errno));
    return "";
  }
  insert(end(), _buffer, _buffer + read_size);

  bi = begin();
  it = bi + before_size;

  for (; it != end(); ++it) {
    if (*it == '\n') {
      std::string line(bi + _readPos, it);
      _readPos += (it - bi) - _readPos + 1;
      return line;
    }
  }
  return "";
}