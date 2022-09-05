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
