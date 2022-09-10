#pragma once

#include <string>
#include <vector>

#define GETLINE_BUFFER_SIZE 10
#define PUBLIC_BUFFER_SIZE 1000000

class GetLine : private std::vector<unsigned char> {
  // Types
 public:
  typedef std::vector<unsigned char> vector;

 private:
  static unsigned char _buffer[GETLINE_BUFFER_SIZE];

 public:
  static unsigned char publicBuffer[PUBLIC_BUFFER_SIZE];

 private:
  int  _fd;
  int  _readPos;
  bool _isRecvError;

  // Constructor
 public:
  GetLine() : _fd(-1), _readPos(0), _isRecvError(false) {}

  // Method
 private:
  // Interface
 public:
  using vector::clear;
  void        setFd(int fd) { _fd = fd; }
  int         getFd() { return _fd; }
  bool        isRecvError() { return _isRecvError; }
  int         remainsCount() { return size() - _readPos; }
  void        initialize();
  std::string nextLine();  // 라인이 완성되지 않으면 "" 리턴
  void        rewindFileOffset();
};
