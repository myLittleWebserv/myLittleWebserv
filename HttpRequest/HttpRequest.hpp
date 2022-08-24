#ifndef HTTPREQUEST_HPP

#include <ctime>
#include <string>
#include <vector>

#include "Storage.hpp"

#define PARSING_TIME_OUT 10000
#define HTTP_DEFAULT_PORT 80

enum HttpRequestParsingState { PARSING_INIT, PARSING_HEADER, PARSING_BODY, PARSING_DONE, BAD_REQUEST, TIME_OUT };

enum MethodType { GET, HEAD, POST, PUT, DELETE };

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
  void     _parseStartLine(const std::string& line);
  void     _parseHeaderField(const std::string& line);
  void     _parseHeader();
  void     _parseHeaderField(const std::string& line);
  void     _parseStartLine(const std::string& line);
  void     _parseBody();
  void     _parseChunk();
  void     _checkTimeOut(clock_t timestamp);
  long int _parseChunkSize(const std::string& line);

  // Constructor
 public:
  HttpRequest()
      : _parsingState(PARSING_INIT),
        _headerSize(0),
        _headerTimeStamp(clock()),
        _bodyTimeStamp(clock()),
        _isBodyExisted(false),
        _isChunked(false) {}

  // Interface
 public:
  bool isEnd() { return _parsingState == PARSING_DONE; }
  bool isConnectionClosed() { return _storage.state() == CONNECTION_CLOSED; }
  bool isBadRequest() { return _parsingState == BAD_REQUEST; }
  void storeChunk(int fd);
};

#endif
