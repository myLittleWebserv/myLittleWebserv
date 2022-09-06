#include "RequestStorage.hpp"

#include <sys/socket.h>

#include "Log.hpp"

void RequestStorage::readFile(int fd) {
  ssize_t read_size = recv(fd, _buffer, READ_BUFFER_SIZE, 0);

  if (read_size == 0) {
    _state = CONNECTION_CLOSED;
  } else if (read_size == READ_BUFFER_SIZE) {
    _state = RECEIVING;
  } else {
    _state = RECEIVE_DONE;
  }

  if (read_size > 0) {
    insert(_buffer, _buffer + read_size);
  }
}

void RequestStorage::dataToBody(Storage::vector& _body, int required_size) {
  _body.insert(_body.end(), begin() + _readPos, begin() + _readPos + required_size);
  _readPos += (required_size + 2);  // jump "\r\n"
}
