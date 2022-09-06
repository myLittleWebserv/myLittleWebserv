// #ifndef STORAGE_HPP
#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#include "Storage.hpp"

enum RequestReadingState { RECEIVE_DONE = -1, CONNECTION_CLOSED = 0, RECEIVING };
// enum FileReadingState { READING, READ_DONE };

class RequestStorage : public Storage {
  // Member Variable
 private:
  enum RequestReadingState _state;

  // Constructor
 public:
  RequestStorage() : _state(RECEIVE_DONE) {}

  // Interface
 public:
  RequestReadingState state() { return _state; }
  void                dataToBody(Storage& _body, int required_size);
  void                readFile(int fd);
};

// #endif
