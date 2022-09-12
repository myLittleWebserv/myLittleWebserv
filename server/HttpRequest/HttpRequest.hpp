#pragma once

#include <ctime>
#include <string>
#include <vector>

#include "Request.hpp"
#include "Storage.hpp"

#define TIME_OUT_HTTP_PARSING 100000
#define HTTP_DEFAULT_PORT 4242
#define HTTP_MAX_HEADER_SIZE 8192

enum HttpRequestParsingState {
  HTTP_PARSING_INIT,
  HTTP_PARSING_HEADER,
  HTTP_PARSING_BODY,
  HTTP_PARSING_DONE,
  HTTP_PARSING_BAD_REQUEST,
  HTTP_PARSING_TIME_OUT,
  HTTP_UPLOADING_CHUNK_INIT,
  HTTP_UPLOADING_CHUNK_SIZE,
  HTTP_UPLOADING_READ_LINE,
  HTTP_UPLOADING_DONE,
  HTTP_UPLOADING_INIT,
  HTTP_PARSING_CONNECTION_CLOSED
};

class HttpRequest : public Request {
  // Member Variable
 private:
  HttpRequestParsingState _parsingState;
  // RequestStorage
  Storage _storage;
  int     _fileFd;
  int     _headerSize;
  size_t  _chunkSize;
  time_t  _timeStamp;  // 생성자에서 초기화
  bool    _isBodyExisted;
  bool    _isChunked;
  bool    _isKeepAlive;
  bool    _serverError;
  int     _secretHeaderForTest;
  size_t  _uploadedSize;
  size_t  _uploadedTotalSize;

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
        _chunkSize(-1),
        _timeStamp(time(NULL)),
        _isBodyExisted(false),
        _isChunked(false),
        _isKeepAlive(true),  //  default: keep-alive: true
        _serverError(false),
        _secretHeaderForTest(0),
        _uploadedSize(0),
        _uploadedTotalSize(0),
        _method(NOT_IMPL),
        _contentLength(0),
        _hostPort(HTTP_DEFAULT_PORT) {}

  // Interface
 public:
  void                    uploadRequest(int recv_fd, int send_fd, clock_t base_clock);
  void                    parseRequest(int recv_fd, clock_t base_clock);
  bool                    isUploadEnd();
  bool                    isParsingEnd();
  bool                    isRecvError();
  bool                    isBadRequest() { return _parsingState == HTTP_PARSING_BAD_REQUEST; }
  bool                    isKeepAlive() { return _isKeepAlive; }
  bool                    isCgi(const std::string& ext);
  void                    initialize();
  int                     hostPort() const { return _hostPort; }
  const std::string&      hostName() const { return _hostName; }
  const std::string&      httpVersion() const { return _httpVersion; }
  int                     contentLength() const { return _contentLength; }
  const std::string&      contentType() const { return _contentType; }
  MethodType              method() const { return _method; }
  const std::string&      uri() const { return _uri; }
  int                     secretHeaderForTest() const { return _secretHeaderForTest; }
  int                     chunkSize() { return _chunkSize; }
  bool                    isChunked() { return _isChunked; }
  size_t                  uploadedTotalSize() { return _uploadedTotalSize; }
  HttpRequestParsingState state() { return _parsingState; }
  // Storage&           storage() { return _storage; }
};
