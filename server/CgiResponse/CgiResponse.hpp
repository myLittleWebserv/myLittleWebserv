#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "CgiStorage.hpp"
#include "Request.hpp"

class HttpRequest;

enum CgiResponseParsingState { CGI_ERROR, CGI_RUNNING, CGI_READING, CGI_PARSING, CGI_PARSING_DONE };

class CgiResponse : public Request {
 public:
  typedef Storage::vector vector;

 private:
  CgiResponseParsingState _parsingState;
  CgiStorage              _storage;
  std::string             _httpVersion;
  int                     _statusCode;
  std::string             _statusMessage;
  enum MethodType         _method;
  std::string             _contentType;
  vector::pointer         _body;
  int                     _secretHeaderForTest;

  std::vector<std::string> _split(const std::string& str, const std::string& delimiter);
  void                     _parseCgiResponse(clock_t base_clock);
  void                     _checkWaitPid(int pid, clock_t clock);

  // Constructor
 public:
  CgiResponse();

  // Interface
 public:
  void               initialize();
  bool               isParsingEnd();
  bool               isExecuteError();
  bool               isReadError();
  void               readCgiResult(int fd, int pid, clock_t clock);
  void               setInfo(const HttpRequest& http_request);
  MethodType         method() const { return _method; }
  const std::string& httpVersion() const { return _httpVersion; }
  int                statusCode() const { return _statusCode; }
  const std::string& statusMessage() const { return _statusMessage; }
  const std::string& contentType() const { return _contentType; }
  int                secretHeaderForTest() const { return _secretHeaderForTest; }
  vector::pointer    body() { return _body; }
  size_t             bodySize() { return _storage.remains(); }
  std::string        CgiResponseResultString();
};

#endif
