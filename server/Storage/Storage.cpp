#include "Storage.hpp"

#include <sys/socket.h>
#include <unistd.h>

unsigned char Storage::_buffer[READ_BUFFER_SIZE];

void Storage::clear() {
  _writePos = 0;
  _readPos  = 0;
}

std::string Storage::getLine() {
  for (vector::size_type i = _readPos; i < size(); ++i) {
    if ((*this)[i] == '\n') {
      std::string line(begin() + _readPos, begin() + i);
      // Log::log()(true, "line", line);
      _readPos = i + 1;
      return line;
    }
  }
  return "";
}

void Storage::moveReadPos(int move) { _readPos += move; }

void Storage::preserveRemains() {
  // if (_readPos != _writePos) {
  //   Log::log()(LOG_LOCATION, "");
  //   for (size_t i = _readPos; i != _writePos; ++i) {
  //     Log::log().getLogStream() << (*this)[i];
  //   }
  // }

  size_t i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _writePos = i - _readPos;
  _readPos  = 0;
}

ssize_t Storage::memToFile(int file_fd, size_t goal_size) {
  size_t  small_one = goal_size < (_writePos - _readPos) ? goal_size : (_writePos - _readPos);
  ssize_t send_size = write(file_fd, data() + _readPos, small_one);

  // Log::log()(true, "goal_size", goal_size);
  // Log::log()(true, "_writePos - _readPos", _writePos - _readPos);
  // Log::log()(true, "small_one", small_one);
  // Log::log()(true, "send_size", send_size);
  // Log::log()(true, "file_fd", file_fd);

  if (send_size == -1) {
    _state = CONNECTION_CLOSED;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }
  _readPos += send_size;
  return send_size;
}
ssize_t Storage::memToSock(int send_fd, size_t goal_size) {
  size_t  small_one = goal_size < (_writePos - _readPos) ? goal_size : (_writePos - _readPos);
  ssize_t send_size = send(send_fd, data() + _readPos, small_one, 0);
  if (send_size == -1) {
    _state = CONNECTION_CLOSED;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }

  // if (goal_size < 100000) {
  //   Log::log()(LOG_LOCATION, "memToSock");
  //   for (int i = 0; i < send_size; ++i) {
  //     Log::log().getLogStream() << *(data() + _readPos + i);
  //   }
  // }
  _readPos += send_size;
  return send_size;
}

ssize_t Storage::fileToMem(int file_fd, size_t goal_size) {
  (void)goal_size;
  ssize_t read_size = read(file_fd, _buffer, READ_BUFFER_SIZE);

  if (read_size == -1) {
    _state = CONNECTION_CLOSED;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }
  insert(_buffer, _buffer + read_size);
  // _fail = false;
  return read_size;
}

void Storage::sockToMem(int recv_fd) {
  ssize_t recv_size = recv(recv_fd, _buffer, READ_BUFFER_SIZE, 0);

  if (recv_size <= 0) {
    _state = CONNECTION_CLOSED;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
  } else if (recv_size == READ_BUFFER_SIZE) {
    _state = RECEIVING;
  } else {
    _state = RECEIVE_DONE;
  }

  if (recv_size > 0) {
    insert(_buffer, _buffer + recv_size);
    Log::log()(_writePos - _readPos > 50000000, "remains", _writePos - _readPos);
  }
}

void Storage::insert(const Storage& storage) {
  vector::const_pointer bi = storage.data() + storage._readPos;
  vector::const_pointer ei = storage.data() + storage._writePos;

  insert(bi, ei);
}

std::string Storage::getLineFile(int fd) {
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      // Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  ssize_t recv_size = read(fd, _buffer, READ_BUFFER_SIZE);
  if (recv_size == -1) {  // ? 0
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    _state = CONNECTION_CLOSED;
    return "";
  }
  insert(_buffer, _buffer + recv_size);
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      // Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  _state = CONNECTION_CLOSED;
  return "";
}
