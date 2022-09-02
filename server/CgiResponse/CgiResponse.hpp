#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CgiStorage.hpp"
#include "Request.hpp"

class HttpRequest;

enum CgiResponseParsingState { CGI_ERROR, CGI_RUNNING, CGI_READING, CGI_PARSING, CGI_PARSING_DONE };

class CgiResponse : public Request {
 private:
  CgiResponseParsingState _parsingState;
  CgiStorage              _storage;
  // bool                       _isParsingEnd;
  // bool                       _isError;
  std::string                _httpVersion;
  int                        _statusCode;
  std::string                _statusMessage;
  enum MethodType            _method;
  std::string                _contentType;
  std::vector<unsigned char> _body;

  std::vector<std::string> _split(const std::string& str, const std::string& delimiter);
  void                     _parseCgiResponse();
  void                     _checkWaitPid(int pid);

  // Constructor
 public:
  CgiResponse();

  // Interface
 public:
  bool                        isParsingEnd();
  bool                        isError();
  void                        readCgiResult(int fd, int pid);
  void                        setInfo(const HttpRequest& http_request);
  MethodType                  method() const { return _method; }
  const std::string&          httpVersion() const { return _httpVersion; }
  int                         statusCode() const { return _statusCode; }
  const std::string&          statusMessage() const { return _statusMessage; }
  const std::string&          contentType() const { return _contentType; }
  std::vector<unsigned char>& body() { return _body; }
  std::string                 CgiResponseResultString();
};

#endif
