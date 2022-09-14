#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "Storage.hpp"

#define DEFAULT_ERROR_PAGE_DIR "error_pages"

struct Event;
struct LocationInfo;
class Request;
class HttpRequest;
class CgiResponse;
class FileManager;

enum HttpResponseStatusCode {
  STATUS_CONTINUE                   = 100,
  STATUS_SWITCHING_PROTOCOLS        = 101,
  STATUS_OK                         = 200,
  STATUS_CREATED                    = 201,
  STATUS_ACCEPTED                   = 202,
  STATUS_NO_CONTENT                 = 204,
  STATUS_MULTIPLE_CHOICES           = 300,
  STATUS_MOVED_PERMANENTLY          = 301,
  STATUS_FOUND                      = 302,
  STATUS_SEE_OTHER                  = 303,
  STATUS_NOT_MODIFIED               = 304,
  STATUS_USE_PROXY                  = 305,
  STATUS_TEMPORARY_REDIRECT         = 307,
  STATUS_BAD_REQUEST                = 400,
  STATUS_PAYMENT_REQUIRED           = 402,
  STATUS_NOT_FOUND                  = 404,
  STATUS_METHOD_NOT_ALLOWED         = 405,
  STATUS_CONFLICT                   = 409,
  STATUS_PAYLOAD_TOO_LARGE          = 413,
  STATUS_INTERNAL_SERVER_ERROR      = 500,
  STATUS_NOT_IMPLEMENTED            = 501,
  STATUS_BAD_GATEWAY                = 502,
  STATUS_SERVICE_UNAVAILABLE        = 503,
  STATUS_GATEWAY_TIMEOUT            = 504,
  STATUS_HTTP_VERSION_NOT_SUPPORTED = 505
};

enum HttpResponseSendingState {
  HTTP_DOWNLOADING_INIT,
  HTTP_SENDING_STORAGE,
  HTTP_SENDING_DONE,
  HTTP_SENDING_CONNECTION_CLOSED,
  HTTP_SENDING_TIME_OUT
};

class HttpResponse {
  // Member Variable
 private:
  enum HttpResponseSendingState _sendingState;

 public:
  Storage                     _storage;
  size_t                      _sentSize;
  enum HttpResponseStatusCode _statusCode;
  size_t                      _goalSize;
  size_t                      _contentLength;
  size_t                      _downloadedSize;
  int                         _fileFd;

  // Constructor
 public:
  ~HttpResponse();
  HttpResponse(HttpResponseSendingState state, const std::string& header, HttpResponseStatusCode status_code,
               size_t content_length = 0, int file_fd = -1);
  HttpResponse(HttpResponseSendingState state, const Storage& storage, const std::string& header,
               HttpResponseStatusCode status_code, size_t content_length = 0, int file_fd = -1);

  // Interface
 public:
  size_t                   contentLength() { return _contentLength; }
  bool                     isConnectionClosed() { return _sendingState == HTTP_SENDING_CONNECTION_CLOSED; }
  bool                     isSendingEnd() { return _sendingState == HTTP_SENDING_DONE; }
  void                     sendResponse(int send_fd);
  int                      fileFd() { return _fileFd; }
  void                     setFileFd(int fd) { _fileFd = fd; }
  HttpResponseStatusCode   statusCode() { return _statusCode; }
  void                     downloadResponse(int recv_fd, clock_t base_clock);
  void                     _headerToSocket(int send_fd);
  HttpResponseSendingState state() { return _sendingState; }
  void                     setState(HttpResponseSendingState state) { _sendingState = state; }
};
