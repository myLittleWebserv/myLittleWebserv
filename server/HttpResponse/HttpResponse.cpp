#include "HttpResponse.hpp"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "CgiResponse.hpp"
#include "Config.hpp"
#include "DataMove.hpp"
#include "Event.hpp"
#include "FileManager.hpp"
#include "GetLine.hpp"
#include "HttpRequest.hpp"
#include "Log.hpp"
#include "syscall.hpp"

// Constructor
HttpResponse::HttpResponse(const std::string& header, HttpResponseStatusCode status_code, size_t content_length,
                           int file_fd)
    : _sendingState(HTTP_SENDING_HEADER),
      _bodySent(0),
      _headerSent(0),
      _header(header),
      _statusCode(status_code),
      _contentLength(content_length),
      _fileFd(file_fd) {
  Log::log()(true, "_contentLength.constuct", _contentLength);
  Log::log()(true, "_fd.constuct", _fileFd);
}

// Interface

void HttpResponse::sendResponse(int recv_fd, int send_fd) {
  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "recv_fd", recv_fd);
  Log::log()(true, "send_fd", send_fd);
  Log::log()(true, "_sendingState", _sendingState);
  switch (_sendingState) {
    case HTTP_SENDING_HEADER:
      DataMove::memToFile(_header.c_str(), _header.size(), send_fd, _headerSent);
      Log::log()(true, "_headerSent", _headerSent);
      if (DataMove::fail()) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        break;
      }
      if (_header.size() != _headerSent) {
        break;
      }
      if (recv_fd == -1) {
        _sendingState = HTTP_SENDING_DONE;
      } else {
        _sendingState = HTTP_SENDING_FILEBODY;
      }

    case HTTP_SENDING_FILEBODY:
      DataMove::fileToFile(recv_fd, send_fd, _bodySent);
      Log::log()(true, "_bodySent", _bodySent);
      if (DataMove::fail()) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        break;
      }
      Log::log()(true, "_bodySent", _bodySent);
      Log::log()(true, "_contentLength", _contentLength);
      if (_contentLength != _bodySent) {
        break;
      }
      _sendingState = HTTP_SENDING_DONE;

    default:
      // close(recv_fd);  // ?
      // close(send_fd);  // ?
      break;
  }
}
