#include "GetLine.hpp"

#include <sys/socket.h>

#include "Log.hpp"
#include "syscall.hpp"

unsigned char GetLine::_buffer[GETLINE_BUFFER_SIZE];
unsigned char GetLine::publicBuffer[PUBLIC_BUFFER_SIZE];

void GetLine::initialize() {
  _readPos     = 0;
  _isRecvError = false;
  clear();
}

void GetLine::rewindFileOffset() { ft::syscall::lseek(_fd, static_cast<off_t>(remainsCount() * -1), SEEK_CUR); }

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
    _isRecvError = true;
    Log::log()(LOG_LOCATION, strerror(errno));
    return "";
  }
  Log::log()(true, "read_size", read_size);
  insert(end(), _buffer, _buffer + read_size);

  bi = begin();
  it = bi + before_size;

  for (; it != end(); ++it) {
    std::cerr << (int)*it << std::endl;
    if (*it == '\n') {
      std::string line(bi + _readPos, it);
      _readPos += (it - bi) - _readPos + 1;

      Log::log()(LOG_LOCATION, "");
      Log::log()(true, "line", std::string(begin() + _readPos, end()));
      Log::log()(true, "before_size", before_size);
      Log::log()(true, "size", size());
      Log::log()(true, "_readPos", _readPos);
      return line;
    }
  }
  return "";
}