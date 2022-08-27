#ifndef STORAGE_HPP

#include <string>
#include <vector>
#define READ_BUFFER_SIZE 8192

enum SocketReadingState { RECEIVE_DONE = -1, CONNECTION_CLOSED = 0, RECEIVING };

class Storage : private std::vector<unsigned char> {
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
  void                                 clear() { this->clear(); }
  SocketReadingState                   state() { return _state; }
  std::vector<unsigned char>::iterator pos() { return begin() + _pos; }
  void                                 readSocket(int fd);
  std::string                          getLine();
};

#endif
