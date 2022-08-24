#include "Storage.hpp"

#include <unistd.h>

void Storage::readSocket(int fd) {
  ssize_t read_size = read(fd, _buffer, READ_BUFFER_SIZE);
  if (read_size >= RECEIVING) {
    insert(end(), _buffer, _buffer + static_cast<ssize_t>(_state));
  }
  _state = static_cast<SocketReadingState>(read_size);
}

std::string Storage::getLine() {
  for (iterator it = _pos; it != end(); ++it) {
    if (*it == '\n') {
      std::string line(_pos, it);
      _pos = ++it;
      return line;
    }
  }
  return "";
}
