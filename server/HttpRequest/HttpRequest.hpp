#ifndef HTTPREQUEST_HPP

#include <ctime>
#include <string>
#include <vector>

#include "Request.hpp"
#include "RequestStorage.hpp"

#define HTTP_PARSING_TIME_OUT 10
#define HTTP_DEFAULT_PORT 4242
#define HTTP_MAX_HEADER_SIZE 8192

enum HttpRequestParsingState {
  HTTP_PARSING_INIT,
  HTTP_PARSING_HEADER,
  HTTP_PARSING_BODY,
  HTTP_PARSING_DONE,
  BAD_REQUEST,
  TIME_OUT
};

class HttpRequest : public Request {
  // Member Variable
 private:
  HttpRequestParsingState    _parsingState;
  RequestStorage             _storage;  // cgi 보낸 후 resize(0);
  std::vector<unsigned char> _body;
  int                        _headerSize;
  time_t                     _headerTimeStamp;  // 생성자에서 초기화
  time_t                     _bodyTimeStamp;    // 헤더 다 읽고 나서 초기화.
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
  void   _parseHeaderField(const std::string& line);
  void   _parseHeader();
  void   _parseBody();
  void   _parseChunk();
  void   _checkTimeOut(time_t timestamp);
  size_t _parseChunkSize(const std::string& line);

  // Constructor
 public:
  HttpRequest()
      : _parsingState(HTTP_PARSING_INIT),
        _headerSize(0),
        _headerTimeStamp(time(NULL)),
        _bodyTimeStamp(_headerTimeStamp),
        _isBodyExisted(false),
        _isChunked(false),
        _chunkSize(-1),
        _isKeepAlive(true),  //  default: keep-alive: true
        _serverError(false),
        _hostPort(HTTP_DEFAULT_PORT) {}

  // Interface
 public:
  bool                              isParsingEnd();
  bool                              isTimeOut() { return _parsingState == TIME_OUT; }
  bool                              isConnectionClosed() { return _storage.state() == CONNECTION_CLOSED; }
  bool                              isBadRequest() { return _parsingState == BAD_REQUEST; }
  bool                              isInternalServerError() { return _serverError; }
  void                              setServerError(bool state) { _serverError = state; }
  bool                              isKeepAlive() { return _isKeepAlive; }
  bool                              isCgi(const std::string& ext);
  void                              storeChunk(int fd);
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
  const Storage&                    storage() { return _storage; }
};

#endif
