#ifndef CGISTORAGE_HPP
#define CGISTORAGE_HPP

#include <unistd.h>

#include <deque>
#include <iostream>
#include <string>

#include "Storage.hpp"

class CgiStorage : public Storage {
  // Member Variable
 private:
  bool _isReadingEnd;
  bool _isReadError;

  // Constructor
 public:
  CgiStorage() : _isReadingEnd(false), _isReadError(false) {}

  // Interface
 public:
  bool isReadError() { return _isReadError; }
  bool isReadingEnd() { return _isReadingEnd; }
  void readFile(int fd);
  // void dataToBody(std::deque<unsigned char>& body, int required_size);
};

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v);

#endif
