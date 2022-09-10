#ifndef CGIRESPONSE_HPP
#define CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <deque>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "GetLine.hpp"
#include "HttpResponse.hpp"
#include "Request.hpp"

class HttpRequest;

enum CgiResponseParsingState { CGI_ERROR, CGI_RUNNING, CGI_READING_HEADER, CGI_PARSING_DONE };

class CgiResponse : public Request {
 private:
  CgiResponseParsingState _parsingState;
  pid_t                   _pid;
  GetLine                 _getLine;
  std::string             _httpVersion;
  int                     _statusCode;
  std::string             _statusMessage;
  enum MethodType         _method;
  std::string             _contentType;
  int                     _bodyFd;
  int                     _bodySize;
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
  bool               isExecuteError() { return _parsingState == CGI_ERROR; }
  bool               isRecvError() { return _getLine.isRecvError(); }
  void               parseRequest(int recv_fd, clock_t base_clock);
  void               setInfo(const HttpRequest& http_request);
  MethodType         method() const { return _method; }
  const std::string& httpVersion() const { return _httpVersion; }
  int                statusCode() const { return _statusCode; }
  const std::string& statusMessage() const { return _statusMessage; }
  const std::string& contentType() const { return _contentType; }
  int                secretHeaderForTest() const { return _secretHeaderForTest; }
  int                bodyFd() { return _bodyFd; }
  size_t             bodySize() { return _bodySize; }
  std::string        CgiResponseResultString();
  GetLine&           getLine() { return _getLine; }
  void               setPid(pid_t pid) { _pid = pid; }
  pid_t              pid() { return _pid; }
};

#endif
