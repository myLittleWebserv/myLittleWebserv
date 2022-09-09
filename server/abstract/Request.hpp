#pragma once

#include <string>

enum MethodType { GET, HEAD, POST, PUT, DELETE, NOT_IMPL };

class Request {
 public:
  virtual const std::string& httpVersion() const = 0;
  virtual MethodType         method() const      = 0;
};