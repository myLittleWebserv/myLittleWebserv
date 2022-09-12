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
HttpResponse::HttpResponse(const std::string& header, HttpResponseStatusCode status_code, size_t content_length,
                           int file_fd)
    : _sendingState(HTTP_SENDING_STORAGE),
      _storage(header.begin(), header.end()),
      _sentSize(0),
      _statusCode(status_code),
      _goalSize(content_length + header.size()),
      _fileFd(file_fd) {}

HttpResponse::HttpResponse(const Storage& storage, const std::string& header, HttpResponseStatusCode status_code,
                           size_t content_length, int file_fd)
    : _sendingState(HTTP_SENDING_STORAGE),
      _storage(header.begin(), header.end()),
      _sentSize(0),
      _statusCode(status_code),
      _goalSize(content_length + header.size()),
      _fileFd(file_fd) {
  _storage.insertBack(storage);
}

// Destructor
HttpResponse::~HttpResponse() {
  if (_fileFd != -1)
    FileManager::registerFileFdToClose(_fileFd);
}

// Interface

void HttpResponse::sendResponse(int recv_fd, int send_fd) {
  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "recv_fd", recv_fd);
  Log::log()(true, "send_fd", send_fd);
  Log::log()(true, "_sendingState", _sendingState);
  ssize_t moved;
  switch (_sendingState) {
    case HTTP_SENDING_STORAGE:
      moved = _storage.dataToSock(recv_fd, send_fd, _goalSize - _sentSize);
      if (moved == -1) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        break;
      }
      _sentSize += moved;
      Log::log()(true, "_sentSize", _sentSize);
      Log::log()(true, "moved", moved);
      Log::log()(true, "_goalSize", _goalSize);
      if (_goalSize == _sentSize)
        _sendingState = HTTP_SENDING_DONE;

    default:
      break;
  }
}
