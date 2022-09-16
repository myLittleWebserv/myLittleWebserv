#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "HttpResponse.hpp"
#include "Request.hpp"
#include "Storage.hpp"

class HttpRequest;

enum CgiResponseParsingState { CGI_ERROR, CGI_RUNNING, CGI_PARSING_HEADER, CGI_PARSING_DONE };

class CgiResponse : public Request {
 private:
  CgiResponseParsingState _parsingState;
  Storage                 _storage;
  pid_t                   _pid;
  std::string             _httpVersion;
  int                     _statusCode;
  std::string             _statusMessage;
  enum MethodType         _method;
  std::string             _contentType;
  int                     _bodyFd;
  int                     _bodySize;
  std::string             _cookies;
  int                     _secretHeaderForTest;

  std::vector<std::string> _split(const std::string& str, const std::string& delimiter);
  void                     _parseCgiResponse(clock_t base_clock);
  bool                     _isCgiExecutionEnd(clock_t clock);
  bool                     _parseLine(const std::string& line);

  // Constructor
 public:
  CgiResponse();

  // Interface
 public:
  void               initialize();
  bool               isParsingEnd() { return _parsingState == CGI_ERROR || _parsingState == CGI_PARSING_DONE; }
  bool               isExecError() { return _parsingState == CGI_ERROR; }
  bool               isRecvError() { return _storage.state() == CONNECTION_CLOSED; }
  void               parseRequest(int recv_fd, clock_t base_clock);
  void               setInfo(const HttpRequest& http_request);
  MethodType         method() const { return _method; }
  const std::string& httpVersion() const { return _httpVersion; }
  int                statusCode() const { return _statusCode; }
  const std::string& statusMessage() const { return _statusMessage; }
  const std::string& contentType() const { return _contentType; }
  const std::string& cookies() const { return _cookies; }
  int                secretHeaderForTest() const { return _secretHeaderForTest; }
  int                bodyFd() { return _bodyFd; }
  size_t             bodySize() { return _bodySize; }
  std::string        CgiResponseResultString();
  void               setPid(pid_t pid) { _pid = pid; }
  pid_t              pid() { return _pid; }
  Storage&           storage() { return _storage; }
};

#endif
