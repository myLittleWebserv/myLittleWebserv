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

  // Constructor
 public:
  CgiStorage() : _isReadingEnd(false) {}

  // Interface
 public:
  bool isReadingEnd() { return _isReadingEnd; }
  void readFile(int fd);
  void dataToBody(std::deque<unsigned char>& body, int required_size);
};

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v);

#endif
