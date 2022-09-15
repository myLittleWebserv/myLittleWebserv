#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#define READ_BUFFER_SIZE 1000000

enum StorageState { RECEIVE_DONE, CONNECTION_CLOSED, RECEIVING };

struct Storage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char> vector;

  // Member Variable
 public:
  static unsigned char _buffer[READ_BUFFER_SIZE];
  size_t               _readPos;
  size_t               _writePos;
  StorageState         _state;
  // using vector::begin;
  // using vector::end;

  // Constructor
 public:
  Storage() : _readPos(0), _writePos(0), _state(RECEIVING) {}
  template <typename BeginIter, typename EndIter>
  Storage(BeginIter bi, EndIter ei);
  virtual ~Storage() {}

  // Interface
 public:
  using vector::data;
  void            clear();
  size_t          size() { return _writePos; }
  size_t          capacity() { return vector::size(); }
  void            moveReadPos(int move);
  int             remains() { return _writePos - _readPos; }
  bool            empty() const { return _writePos == _readPos; }
  size_t          readPos() { return _readPos; }
  void            setReadPos(size_t pos) { _readPos = pos; }
  vector::pointer currentReadPos() { return data() + _readPos; }
  vector::pointer currentWritePos() { return data() + _writePos; }
  std::string     getLine();
  std::string     getLineFile(int fd);
  ssize_t         memToSock(int send_fd, size_t goal_size);
  ssize_t         memToFile(int send_fd, size_t goal_size);
  ssize_t         fileToMem(int recv_fd, size_t goal_size);
  void            sockToMem(int recv_fd);
  void            preserveRemains();
  template <typename BeginIter, typename EndIter>
  void         insert(BeginIter bi, EndIter ei);
  void         insert(const Storage& storage);
  StorageState state() { return _state; }
};

template <typename BeginIter, typename EndIter>
void Storage::insert(BeginIter bi, EndIter ei) {
  // Log::log()(LOG_LOCATION, "");
  // Log::log()(_writePos > 10000000, "_writePos", _writePos);
  // Log::log()(true, "capacity()", capacity());
  // Log::log()(true, "bi", bi);
  // Log::log()(true, "ei", ei);
  for (; _writePos != capacity() && bi != ei; ++_writePos, ++bi) {
    (*this)[_writePos] = *bi;
  }
  vector::insert(vector::end(), bi, ei);
  _writePos += (ei - bi);
}

template <typename BeginIter, typename EndIter>
Storage::Storage(BeginIter bi, EndIter ei)
    : vector(bi, ei), _readPos(0), _writePos(vector::size()), _state(RECEIVING) {}