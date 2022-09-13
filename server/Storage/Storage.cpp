#include "Storage.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include "Log.hpp"

unsigned char Storage::_buffer[READ_BUFFER_SIZE];
unsigned char Storage::publicBuffer[PUBLIC_BUFFER_SIZE];

Storage::Storage(const Storage& storage) : _readPos(0), _writePos(0) {
  vector::const_iterator begin = storage.begin() + storage._readPos;
  vector::const_iterator end   = storage.end() + storage._writePos;

  insertBack(begin, end);
}

void Storage::insertBack(const Storage& storage) {
  vector::const_pointer bi = storage.data() + storage._readPos;
  vector::const_pointer ei = storage.data() + storage._writePos;

  insertBack(bi, ei);
}

void Storage::clear() {
  _writePos = 0;
  _readPos  = 0;
  _fail     = false;
}

std::string Storage::getLineSock(int fd) {
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
  insertBack(_buffer, _buffer + recv_size);
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  _fail = false;
  return "";
}

std::string Storage::getLineFile(int fd) {
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  ssize_t recv_size = read(fd, _buffer, READ_BUFFER_SIZE);
  if (recv_size == -1) {  // ? 0
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    _fail = true;
    return "";
  }
  insertBack(_buffer, _buffer + recv_size);
  for (size_t i = _readPos; i != _writePos; ++i) {
    if ((*this)[i] == '\n') {
      std::string line(data() + _readPos, data() + i);
      Log::log()(LOG_LOCATION, line);
      _readPos = i + 1;
      return line;
    }
  }
  _fail = false;
  return "";
}

void Storage::preserveRemains() {
  size_t i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _writePos = i - _readPos;
  _readPos  = 0;
  vector::resize(_writePos);
}

ssize_t Storage::dataToFile(int recv_fd, int send_fd, size_t goal_size) {
  if (_readPos != _writePos) {
    Log::log()(LOG_LOCATION, "(INIT) memToFile");
    return memToFile(send_fd, goal_size);
  } else {
    clear();
    Log::log()(LOG_LOCATION, "(INIT) sockToFile");
    return sockToFile(recv_fd, send_fd, goal_size);
  }
}

ssize_t Storage::dataToSock(int recv_fd, int send_fd, size_t goal_size) {
  if (_readPos != _writePos) {
    Log::log()(LOG_LOCATION, "(INIT) memToFile");
    return memToFile(send_fd, goal_size);
  } else {
    clear();
    Log::log()(LOG_LOCATION, "(INIT) fileToSock");
    return fileToSock(recv_fd, send_fd, goal_size);
  }
}

ssize_t Storage::fileToSock(int recv_fd, int send_fd, size_t goal_size) {
  Log::log()(true, "recv_fd", recv_fd);
  Log::log()(true, "send_fd", send_fd);
  Log::log()(true, "goal_size", goal_size);

  size_t  small_one = goal_size < PUBLIC_BUFFER_SIZE ? goal_size : PUBLIC_BUFFER_SIZE;
  ssize_t recv_size = read(recv_fd, publicBuffer, small_one);  // ret -> 0, eof
  if (recv_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }

  if (recv_size == 0) {
    Log::log()(true, "empty file", "");
  }

  ssize_t send_size = send(send_fd, publicBuffer, recv_size, 0);
  if (send_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }

  Log::log()(true, "recv_size", recv_size);
  Log::log()(true, "send_size", send_size);

  insertBack(Storage::publicBuffer + send_size, Storage::publicBuffer + recv_size);
  _fail = false;
  return send_size;
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
  insertBack(Storage::publicBuffer + send_size, Storage::publicBuffer + recv_size);
  _fail = false;
  return send_size;
}

ssize_t Storage::memToFile(int file_fd, size_t goal_size) {
  size_t  small_one = goal_size < (_writePos - _readPos) ? goal_size : (_writePos - _readPos);
  ssize_t send_size = write(file_fd, data() + _readPos, small_one);
  if (send_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return -1;
  }
  _readPos += send_size;
  _fail = false;
  return send_size;
}