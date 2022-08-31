#include "Storage.hpp"

#include <sys/socket.h>

#include "Log.hpp"

void Storage::readSocket(int fd) {
  ssize_t read_size = recv(fd, _buffer, READ_BUFFER_SIZE, 0);

  if (read_size == 0) {
    _state = CONNECTION_CLOSED;
  } else if (read_size == READ_BUFFER_SIZE) {
    _state = RECEIVING;
  } else {
    _state = RECEIVE_DONE;
  }

  if (read_size > 0) {
    insert(end(), _buffer, _buffer + read_size);
  }
}

std::string Storage::getLine() {
  for (vector::size_type i = _pos; i < size(); ++i) {
    if ((*this)[i] == '\n') {
      std::string line(begin() + _pos, begin() + i);
      _pos = i + 1;
      return line;
    }
  }
  return "";
}

void Storage::dataToBody(vector& _body, int required_size) {
  _body.insert(_body.end(), begin() + _pos, begin() + _pos + required_size);
  _pos += (required_size + 2);  // jump "\r\n"
}