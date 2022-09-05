#include "Storage.hpp"

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

void Storage::preserveRemains() {
  int i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _readPos  = 0;
  _writePos = i;
}