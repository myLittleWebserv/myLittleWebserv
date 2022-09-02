#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CgiStorage.hpp"

class CgiResponse {
 private:
  CgiStorage                 _storage;
  bool                       _isParsingEnd;
  bool                       _isError;
  int                        _statusCode;
  std::string                _statusMessage;
  std::string                _contentType;
  std::vector<unsigned char> _body;

  std::vector<std::string> _split(const std::string& str, const std::string& delimiter);
  void                     _parseCgiResponse();

  // Constructor
 public:
  CgiResponse();

  // Interface
 public:
  bool                              isParsingEnd();
  bool                              isError();
  void                              readCgiResult(int fd, int pid);
  int                               getStatusCode();
  std::string                       getStatusMessage();
  std::string                       getContentType();
  const std::vector<unsigned char>& getBody();
  std::string                       CgiResponseResultString();
};

#endif
