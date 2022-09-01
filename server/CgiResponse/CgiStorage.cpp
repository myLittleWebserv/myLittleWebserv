#include "CgiStorage.hpp"

void CgiStorage::readFd(int fd) {
  ssize_t read_size = read(fd, _buffer, READ_BUFFER_SIZE);

  if (read_size == 0) {
    _isReadingEnd = true;
  }
  if (read_size > 0) {
    insert(end(), _buffer, _buffer + read_size);
  }
}

std::string CgiStorage::remainder() {
  return std::string(begin() + _pos, end());
}

std::string CgiStorage::getLine() {
  for (vector::size_type i = _pos; i < size(); ++i) {
    if ((*this)[i] == '\n') {
      std::string line(begin() + _pos, begin() + i);
      _pos = i + 1;
      return line;
    }
  }
  return "";
}

bool CgiStorage::toBody(vector& _body, int required_size) {
  if (required_size > static_cast<int>(size()) - _pos) {
    return false;
  }
  _body.insert(_body.end(), begin() + _pos, begin() + _pos + required_size);
  _pos += required_size;
  _pos += 2;  // jump "\r\n"
  return true;
}
