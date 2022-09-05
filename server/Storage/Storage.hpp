// #ifndef STORAGE_HPP
#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#define READ_BUFFER_SIZE 8192

class Storage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char> vector;

  // Member Variable
 protected:
  unsigned char _buffer[READ_BUFFER_SIZE];
  size_t        _readPos;
  size_t        _writePos;
  using vector::begin;

  // Constructor
 public:
  Storage() : _readPos(0), _writePos(0) {}
  virtual ~Storage() {}

  // Interface
 public:
  size_t          size() { return _writePos; }
  size_t          capacity() { return vector::size(); }
  void            moveReadPos(int move) { _readPos += move; }
  int             remains() { return _writePos - _readPos; }
  bool            empty() const { return _writePos == _readPos; }
  vector::pointer currentReadPos() { return data() + _readPos; }
  vector::pointer currentWritePos() { return data() + _writePos; }
  std::string     getLine();
  void            preserveRemains();
  template <typename BeginIter, typename EndIter>
  void insert(BeginIter bi, EndIter ei);
};

template <typename BeginIter, typename EndIter>
void Storage::insert(BeginIter bi, EndIter ei) {
  for (; _writePos != capacity() && bi != ei; ++_writePos, ++bi) {
    (*this)[_writePos] = *bi;
  }
  for (; bi != ei; ++bi) {
    push_back(*bi);
    ++_writePos;
  }
}

// #endif
