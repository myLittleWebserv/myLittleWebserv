#ifndef CGISTORAGE_HPP
#define CGISTORAGE_HPP

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#define READ_BUFFER_SIZE 8192

class CgiStorage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char>           vector;
  typedef std::vector<unsigned char>::iterator iterator;

  // Member Variable
 private:
  bool          _isReadingEnd;
  unsigned char _buffer[READ_BUFFER_SIZE];
  int           _pos;

  // Constructor
 public:
  CgiStorage() : _isReadingEnd(false), _pos(0) {}

  // Interface
 public:
  using vector::clear;
  using vector::size;
  bool        isReadingEnd() { return _isReadingEnd; }
  bool        toBody(vector& _body, int required_size);
  void        readFd(int fd);
  vector      remainder();
  std::string getLine();
};

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v);

#endif
