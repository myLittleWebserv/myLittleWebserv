#ifndef HTTPREQUEST_HPP

#include <ctime>
#include <string>
#include <vector>

#include "Storage.hpp"

#define PARSING_TIME_OUT 10000
#define HTTP_DEFAULT_PORT 80
#define HTTP_MAX_HEADER_SIZE 8192

enum HttpRequestParsingState { PARSING_INIT, PARSING_HEADER, PARSING_BODY, PARSING_DONE, BAD_REQUEST, TIME_OUT };

enum MethodType { GET, HEAD, POST, PUT, DELETE, NOT_IMPL };

class HttpRequest {
  // Member Variable
 private:
  HttpRequestParsingState    _parsingState;
  Storage                    _storage;  // cgi 보낸 후 resize(0);
  std::vector<unsigned char> _body;
  int                        _headerSize;
  clock_t                    _headerTimeStamp;  // 생성자에서 초기화
  clock_t                    _bodyTimeStamp;    // 헤더 다 읽고 나서 초기화.
  bool                       _isBodyExisted;
  bool                       _isChunked;
  int                        _chunkSize;
  bool                       _isKeepAlive;

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
  void   _checkTimeOut(clock_t timestamp);
  size_t _parseChunkSize(const std::string& line);

  // Constructor
 public:
  HttpRequest()
      : _parsingState(PARSING_INIT),
        _headerSize(0),
        _headerTimeStamp(clock()),
        _bodyTimeStamp(_headerTimeStamp),
        _isBodyExisted(false),
        _isChunked(false),
        _chunkSize(-1),
        _isKeepAlive(false),  //  default: close
        _hostPort(HTTP_DEFAULT_PORT) {}

  // Interface
 public:
  bool isEnd() { return _parsingState == PARSING_DONE || _parsingState == BAD_REQUEST || _parsingState == TIME_OUT; }
  bool isTimeOut() { return _parsingState == TIME_OUT; }
  bool isConnectionClosed() { return _storage.state() == CONNECTION_CLOSED; }
  bool isBadRequest() { return _parsingState == BAD_REQUEST; }
  bool isKeepAlive() { return _isKeepAlive; }
  bool isCgi(const std::string& ext);
  void storeChunk(int fd);
  void initialize();
  int  hostPort() { return _hostPort; }
  const std::string&                hostName() { return _hostName; }
  const std::string&                httpVersion() { return _httpVersion; }
  int                               contentLength() { return _contentLength; }
  const std::string&                contentType() { return _contentType; }
  MethodType                        method() { return _method; }
  const std::string&                uri() { return _uri; }
  const std::vector<unsigned char>& body() { return _body; }
};

#endif
