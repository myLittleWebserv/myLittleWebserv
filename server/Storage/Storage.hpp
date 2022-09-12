#pragma once

#include <unistd.h>

#include <string>
#include <vector>

#include "Log.hpp"
#define READ_BUFFER_SIZE 1000
#define PUBLIC_BUFFER_SIZE 1000000

class Storage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char> vector;

  // Member Variable
 public:
  static unsigned char _buffer[READ_BUFFER_SIZE];
  static unsigned char publicBuffer[PUBLIC_BUFFER_SIZE];
  size_t               _readPos;
  size_t               _writePos;
  bool                 _fail;
  // using vector::begin;

  // Constructor
 public:
  Storage() : _readPos(0), _writePos(0) {}
  virtual ~Storage() {}

  // Interface
 public:
  using vector::data;
  void            clear();
  size_t          size() { return _writePos; }
  size_t          capacity() { return vector::size(); }
  void            moveReadPos(ssize_t move);
  int             remains() { return _writePos - _readPos; }
  bool            empty() const { return _writePos == _readPos; }
  vector::pointer currentReadPos() { return data() + _readPos; }
  vector::pointer currentWritePos() { return data() + _writePos; }
  std::string     getLine(int fd);
  void            preserveRemains();
  bool            fail() { return _fail; }
  ssize_t         memToFile(int file_fd, size_t goal_size);
  ssize_t         sockToFile(int recv_fd, int send_fd, size_t goal_size);
  ssize_t         dataToFile(int recv_fd, int send_fd, size_t goal_size);
  template <typename BeginIter, typename EndIter>
  void insert(BeginIter bi, EndIter ei);
};

template <typename BeginIter, typename EndIter>
void Storage::insert(BeginIter bi, EndIter ei) {
  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "_writePos", _writePos);
  Log::log()(true, "capacity()", capacity());
  // Log::log()(true, "bi", bi);
  // Log::log()(true, "ei", ei);
  for (; _writePos != capacity() && bi != ei; ++_writePos, ++bi) {
    (*this)[_writePos] = *bi;
  }
  vector::insert(vector::end(), bi, ei);
  _writePos += (ei - bi);
}
