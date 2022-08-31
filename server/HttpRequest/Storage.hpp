// #ifndef STORAGE_HPP
#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#define READ_BUFFER_SIZE 8192

enum SocketReadingState { RECEIVE_DONE = -1, CONNECTION_CLOSED = 0, RECEIVING };

class Storage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char>           vector;
  typedef std::vector<unsigned char>::iterator iterator;

  // Member Variable
 private:
  SocketReadingState _state;
  unsigned char      _buffer[READ_BUFFER_SIZE];
  int                _pos;

  // Constructor
 public:
  Storage() : _state(RECEIVE_DONE), _pos(0) {}

  // Interface
 public:
  using vector::begin;
  using vector::clear;
  using vector::data;
  using vector::end;
  using vector::insert;
  using vector::size;
  SocketReadingState state() { return _state; }
  int                remains() { return static_cast<int>(size()) - _pos; }
  bool               empty() const { return static_cast<int>(size()) == _pos; }
  void               dataToBody(vector& _body, int required_size);
  void               readSocket(int fd);
  void               movePos(int move) { _pos += move; }
  std::string        getLine();
};

// #endif
