#pragma once

#include <string>
struct Event;

enum MethodType { GET, HEAD, POST, PUT, DELETE, NOT_IMPL };

class Request {
 public:
  virtual const std::string& httpVersion() const        = 0;
  virtual MethodType         method() const             = 0;
  virtual void               parseRequest(Event& event) = 0;
};