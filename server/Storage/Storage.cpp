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

void Storage::preserveRemains() {
  vector::iterator b = begin();
  int              i = 0;
  for (vector::iterator it = b + _pos; it != end(); ++it, ++i) {
    *(b + i) = *it;
  }
  Log::log()(LOG_LOCATION, "", ALL);
  Log::log()(true, "pos", _pos);
  Log::log()(true, "size", size());
  resize(size() - _pos);
  _pos = 0;
}