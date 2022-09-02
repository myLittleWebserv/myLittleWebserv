#ifndef CGISTORAGE_HPP
#define CGISTORAGE_HPP

#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "Storage.hpp"

#define READ_BUFFER_SIZE 8192

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
  void dataToBody(vector& body, int required_size);
};

std::ostream& operator<<(std::ostream& os, const std::vector<unsigned char>& v);

#endif