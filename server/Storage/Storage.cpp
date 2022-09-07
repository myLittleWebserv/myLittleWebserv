#include "Storage.hpp"

unsigned char Storage::_buffer[READ_BUFFER_SIZE];

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
  Log::log()(LOG_LOCATION, "preserve remains2");
  for (size_t j = 0; j < capacity(); ++j) {
    Log::log().getLogStream() << (*this)[j];
  }

  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "readPos", _readPos);
  Log::log()(true, "writePos", _writePos);
  size_t i;
  for (i = _readPos; i != _writePos; ++i) {
    (*this)[i - _readPos] = (*this)[i];
  }
  _writePos = i - _readPos;
  _readPos  = 0;

  Log::log()(LOG_LOCATION, "preserve remains3");
  for (size_t j = 0; j < capacity(); ++j) {
    Log::log().getLogStream() << (*this)[j];
  }

  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "readPos", _readPos);
  Log::log()(true, "writePos", _writePos);
}