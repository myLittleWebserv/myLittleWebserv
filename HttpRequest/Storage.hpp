#ifndef STORAGE_HPP

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
  using vector::clear;
  using vector::size;
  SocketReadingState state() { return _state; }
  iterator           pos() { return begin() + _pos; }
  void               readSocket(int fd);
  std::string        getLine();
};

#endif
