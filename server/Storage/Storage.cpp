#include "Storage.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include "Log.hpp"

unsigned char Storage::_buffer[READ_BUFFER_SIZE];
unsigned char Storage::publicBuffer[PUBLIC_BUFFER_SIZE];

void Storage::clear() {
  _writePos = 0;
  _readPos  = 0;
  _fail     = false;
}

std::string Storage::getLine(int fd) {
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  ssize_t recv_size = recv(fd, _buffer, READ_BUFFER_SIZE, 0);
  if (recv_size == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    return "";
  }
  if (recv_size == 0) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    _fail = true;
    return "";
  }
  insert(_buffer, _buffer + recv_size);
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  return "";
}

void Storage::moveReadPos(ssize_t move) {
  if (move >= 0)
    _readPos += move;
  else
    Log::log()(LOG_LOCATION, "WRONG MOVE POS");
}

void Storage::preserveRemains() {
  size_t i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _writePos = i - _readPos;
  _readPos  = 0;
}

ssize_t Storage::dataToFile(int recv_fd, int send_fd, size_t goal_size) {
  if (_readPos != _writePos) {
    Log::log()(LOG_LOCATION, "(INIT) memToFile");
    return memToFile(send_fd, goal_size);
  } else {
    Log::log()(LOG_LOCATION, "(INIT) sockFile");
    return sockToFile(recv_fd, send_fd, goal_size);
  }
}

ssize_t Storage::sockToFile(int recv_fd, int send_fd, size_t goal_size) {
  Log::log()(true, "recv_fd", recv_fd);
  Log::log()(true, "send_fd", send_fd);
  Log::log()(true, "goal_size", goal_size);

  ssize_t recv_size = recv(recv_fd, publicBuffer, goal_size, 0);
  if (recv_size == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return 0;
  }

  ssize_t send_size = write(send_fd, publicBuffer, recv_size);
  if (send_size == -1 || recv_size == 0) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }
  insert(Storage::publicBuffer + send_size, Storage::publicBuffer + recv_size);
  _fail = false;
  return send_size;
}

ssize_t Storage::memToFile(int file_fd, size_t goal_size) {
  size_t  to_write  = goal_size < _writePos - _readPos ? goal_size : _writePos - _readPos;
  ssize_t send_size = write(file_fd, data() + _readPos, to_write);
  if (send_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }
  _readPos += send_size;
  _fail = false;
  return send_size;
}