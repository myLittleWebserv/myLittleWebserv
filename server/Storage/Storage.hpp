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
  Storage(const Storage& storage);
  template <typename BeginIter, typename EndIter>
  Storage(BeginIter bi, EndIter ei);

  // Interface
 public:
  void        print();
  void        clear();
  size_t      size() { return _writePos; }
  size_t      capacity() { return vector::size(); }
  int         remains() { return _writePos - _readPos; }
  bool        empty() const { return _writePos == _readPos; }
  std::string getLineSock(int fd);
  std::string getLineFile(int fd);
  void        preserveRemains();
  bool        fail() { return _fail; }
  ssize_t     memToFile(int file_fd, size_t goal_size);
  ssize_t     sockToFile(int recv_fd, int send_fd, size_t goal_size);
  ssize_t     fileToSock(int recv_fd, int send_fd, size_t goal_size);
  ssize_t     dataToFile(int recv_fd, int send_fd, size_t goal_size);
  ssize_t     dataToSock(int recv_fd, int send_fd, size_t goal_size);
  void        insertBack(const Storage& storage);
  template <typename BeginIter, typename EndIter>
  void insertBack(BeginIter bi, EndIter ei);
};

template <typename BeginIter, typename EndIter>
Storage::Storage(BeginIter bi, EndIter ei) : vector(bi, ei), _readPos(0) {
  _writePos = vector::size();
}

template <typename BeginIter, typename EndIter>
void Storage::insertBack(BeginIter bi, EndIter ei) {
  // Log::log()(LOG_LOCATION, "");
  // Log::log()(true, "_writePos", _writePos);
  // Log::log()(true, "capacity()", capacity());
  for (; _writePos != capacity() && bi != ei; ++_writePos, ++bi) {
    (*this)[_writePos] = *bi;
  }
  vector::insert(vector::end(), bi, ei);
  _writePos += (ei - bi);
}
