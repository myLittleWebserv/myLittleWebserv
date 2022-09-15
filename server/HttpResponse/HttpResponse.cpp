#include "HttpResponse.hpp"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "CgiResponse.hpp"
#include "Config.hpp"
#include "Event.hpp"
#include "FileManager.hpp"
#include "HttpRequest.hpp"
#include "Log.hpp"
#include "syscall.hpp"

// Constructor
HttpResponse::HttpResponse(HttpResponseType type, HttpResponseSendingState state, const std::string& header,
                           HttpResponseStatusCode status_code, size_t content_length, int file_fd)
    : _sendingState(state),
      _type(type),
      _storage(header.begin(), header.end()),
      _sentSize(0),
      _statusCode(status_code),
      _goalSize(content_length + header.size()),
      _contentLength(content_length),
      _downloadedSize(0),
      _fileFd(file_fd) {}

HttpResponse::HttpResponse(HttpResponseType type, HttpResponseSendingState state, const Storage& storage,
                           const std::string& header, HttpResponseStatusCode status_code, size_t content_length,
                           int file_fd)
    : _sendingState(state),
      _type(type),
      _storage(header.begin(), header.end()),
      _sentSize(0),
      _statusCode(status_code),
      _goalSize(content_length + header.size()),
      _contentLength(content_length),
      _downloadedSize(0),
      _fileFd(file_fd) {
  _storage.insert(storage);
}

// Destructor
HttpResponse::~HttpResponse() {}

// Interface

void HttpResponse::sendResponse(int send_fd) {
  ssize_t moved;
  switch (_sendingState) {
    case HTTP_SENDING_STORAGE:
      moved = _storage.memToSock(send_fd, _goalSize - _sentSize);
      if (moved == -1) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        break;
      }
      _sentSize += moved;
      if (_goalSize == _sentSize)
        _sendingState = HTTP_SENDING_DONE;
      Log::log()(_goalSize > 10000000 && _sentSize > 50000000, "_sentSize", _sentSize);

    default:
      break;
  }
}

void HttpResponse::downloadResponse(int recv_fd, clock_t base_clock) {
  (void)base_clock;
  std::string line;
  ssize_t     moved;

  switch (_sendingState) {
    case HTTP_DOWNLOADING_INIT:
      moved = _storage.fileToMem(recv_fd, _contentLength - _downloadedSize);
      if (moved == -1) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        break;
      }
      _downloadedSize += moved;
      if (_contentLength == _downloadedSize)
        _sendingState = HTTP_SENDING_STORAGE;
      else
        _sendingState = HTTP_DOWNLOADING_INIT;

    default:
      break;
  }
}
