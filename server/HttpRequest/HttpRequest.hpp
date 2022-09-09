#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "GetLine.hpp"
#include "Request.hpp"
#include "RequestStorage.hpp"

#define TIME_OUT_HTTP_PARSING 10
#define HTTP_DEFAULT_PORT 4242
#define HTTP_MAX_HEADER_SIZE 8192

enum HttpRequestParsingState {
  HTTP_PARSING_INIT,
  HTTP_PARSING_HEADER,
  HTTP_PARSING_BODY,
  HTTP_PARSING_BODY_CHUNK,
  HTTP_PARSING_DONE,
  HTTP_PARSING_BAD_REQUEST,
  HTTP_PARSING_TIME_OUT
};

class HttpRequest : public Request {
  // Member Variable
 private:
  HttpRequestParsingState _parsingState;
  // RequestStorage             _storage;
  GetLine                    _getLine;
  std::vector<unsigned char> _body;
  int                        _bodyFd;
  int                        _headerSize;
  int                        _bodySize;
  time_t                     _timeStamp;  // 생성자에서 초기화
  bool                       _isBodyExisted;
  bool                       _isChunked;
  int                        _chunkSize;
  bool                       _isKeepAlive;
  bool                       _serverError;
  int                        _secretHeaderForTest;

  // HttpRequest Variable
  MethodType  _method;
  std::string _uri;
  std::string _httpVersion;
  int         _contentLength;
  std::string _contentType;
  int         _hostPort;
  std::string _hostName;

  // Method
 private:
  void   _parseStartLine(const std::string& line);
  bool   _parseHeaderField(const std::string& line);
  void   _parseChunk();
  void   _checkTimeOut();
  size_t _parseChunkSize(const std::string& line);

  // Constructor
 public:
  HttpRequest()
      : _parsingState(HTTP_PARSING_INIT),
        _headerSize(0),
        _bodySize(0),
        _timeStamp(time(NULL)),
        _isBodyExisted(false),
        _isChunked(false),
        _chunkSize(-1),
        _isKeepAlive(true),  //  default: keep-alive: true
        _serverError(false),
        _method(NOT_IMPL),
        _contentLength(0),
        _hostPort(HTTP_DEFAULT_PORT) {}

  // Interface
 public:
  void                              parseRequest(Event& event);
  bool                              isParsingEnd();
  bool                              isTimeOut() { return _parsingState == HTTP_PARSING_TIME_OUT; }
  bool                              isConnectionClosed() { return _getLine.isReadError(); }
  bool                              isBadRequest() { return _parsingState == HTTP_PARSING_BAD_REQUEST; }
  bool                              isInternalServerError() { return _serverError; }
  void                              setServerError(bool state) { _serverError = state; }
  bool                              isKeepAlive() { return _isKeepAlive; }
  bool                              isCgi(const std::string& ext);
  void                              initialize();
  int                               hostPort() const { return _hostPort; }
  const std::string&                hostName() const { return _hostName; }
  const std::string&                httpVersion() const { return _httpVersion; }
  int                               contentLength() const { return _contentLength; }
  const std::string&                contentType() const { return _contentType; }
  MethodType                        method() const { return _method; }
  const std::string&                uri() const { return _uri; }
  int                               secretHeaderForTest() const { return _secretHeaderForTest; }
  const std::vector<unsigned char>& body() { return _body; }
  int                               bodyFd() { return _bodyFd; }
};
