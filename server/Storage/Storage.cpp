#include "Storage.hpp"

std::string Storage::getLine() {
  for (vector::size_type i = _pos; i < size(); ++i) {
    if ((*this)[i] == '\n') {
      std::string line(begin() + _pos, begin() + i);
      _pos = i + 1;
      return line;
    }
  }
  return "";
}
