#include "Storage.hpp"

unsigned char Storage::_buffer[READ_BUFFER_SIZE];

void Storage::clear() {
  _writePos = 0;
  _readPos  = 0;
}

std::string Storage::getLine() {
  for (vector::size_type i = _readPos; i < size(); ++i) {
    if ((*this)[i] == '\n') {
      std::string line(begin() + _readPos, begin() + i);
      _readPos = i + 1;
      return line;
    }
  }
  return "";
}

void Storage::moveReadPos(int move) {
  // if (move + _readPos > _writePos)
  //   _readPos = _writePos;
  // else
  _readPos += move;
}

void Storage::preserveRemains() {
  size_t i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _writePos = i - _readPos;
  _readPos  = 0;
}