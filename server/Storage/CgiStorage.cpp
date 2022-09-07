#include "CgiStorage.hpp"

void CgiStorage::readFile(int fd) {
  ssize_t read_size = read(fd, _buffer, READ_BUFFER_SIZE);

  if (read_size < READ_BUFFER_SIZE) {
    _isReadingEnd = true;
  }
  if (read_size > 0) {
    insert(_buffer, _buffer + read_size);
  }
}

// void CgiStorage::dataToBody(std::deque<unsigned char>& body, int required_size) {
//   body.insert(body.end(), begin() + _readPos, begin() + _readPos + required_size);
//   _readPos += required_size;
// }

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v) {
  for (std::vector<unsigned char>::const_iterator it = v.begin(); it != v.end(); ++it) {
    os << *it;
  }
  return os;
}