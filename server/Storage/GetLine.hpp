#pragma once

#include <string>
#include <vector>

#define GETLINE_BUFFER_SIZE 10

class GetLine : private std::vector<unsigned char> {
  // Types
 public:
  typedef std::vector<unsigned char> vector;

 private:
  static unsigned char _buffer[GETLINE_BUFFER_SIZE];

 private:
  int  _fd;
  int  _readPos;
  bool _isReadError;

  // Constructor
 public:
  GetLine() : _fd(-1), _readPos(0), _isReadError(false) {}

  // Method
 private:
  // Interface
 public:
  void        setFd(int fd) { _fd = fd; }
  int         getFd() { return _fd; }
  bool        isReadError() { return _isReadError; }
  int         remainsCount() { return size() - _readPos; }
  void        initialize();
  std::string nextLine();  // 라인이 완성되지 않으면 "" 리턴
};
