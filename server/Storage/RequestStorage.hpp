#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#include "Storage.hpp"

enum RequestReadingState { RECEIVE_DONE = -1, CONNECTION_CLOSED = 0, RECEIVING };

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
  void                dataToBody(vector& _body, int required_size, bool chunked = 0);
  void                readFile(int fd);
};
