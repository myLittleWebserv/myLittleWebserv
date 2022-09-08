#include "CgiStorage.hpp"

void CgiStorage::readFile(int fd) {
  ssize_t read_size = read(fd, _buffer, READ_BUFFER_SIZE);

  if (read_size == -1) {
    _isReadError = true;
  } else if (read_size == READ_BUFFER_SIZE) {
    _isReadingEnd = false;
  } else {
    _isReadingEnd = true;
  }
  if (read_size > 0) {
    insert(_buffer, _buffer + read_size);
  }
}

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v) {
  for (std::vector<unsigned char>::const_iterator it = v.begin(); it != v.end(); ++it) {
    os << *it;
  }
  return os;
}