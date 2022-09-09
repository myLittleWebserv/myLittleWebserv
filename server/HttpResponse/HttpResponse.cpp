#include "HttpResponse.hpp"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <sstream>

#include "CgiResponse.hpp"
#include "Config.hpp"
#include "FileManager.hpp"
#include "GetLine.hpp"
#include "HttpRequest.hpp"
#include "syscall.hpp"

// Constructor

HttpResponse::HttpResponse(HttpRequest& request, LocationInfo& location_info)
    : _sendingState(HTTP_SENDING_HEADER), _bodyFd(-1), _bodySent(0), _headerSent(0), _statusCode(0), _contentLength(0) {
  if (request.isBadRequest()) {
    _makeErrorResponse(400, request, location_info);
    return;
  }
  if (request.httpVersion() != "HTTP/1.1") {
    _makeErrorResponse(505, request, location_info);
    return;
  }
  if (request.method() == NOT_IMPL) {
    _makeErrorResponse(501, request, location_info);
    return;
  }
  if (!_isAllowedMethod(request.method(), location_info.allowedMethods)) {
    _makeErrorResponse(405, request, location_info);
    return;
  }
  if (request.isInternalServerError()) {
    _makeErrorResponse(500, request, location_info);
    return;
  }
  if (static_cast<int>(request.body().size()) > location_info.maxBodySize) {
    _makeErrorResponse(413, request, location_info);
    return;
  }

  if (location_info.redirStatus != -1) {
    _makeRedirResponse(location_info.redirStatus, request, location_info);
    return;
  }

  switch (request.method()) {
    case GET:
      _processGetRequest(request, location_info);
      break;
    case HEAD:
      _processHeadRequest(request, location_info);
      break;
    case POST:
      _processPostRequest(request, location_info);
      break;
    case PUT:
      _processPutRequest(request, location_info);
      break;
    case DELETE:
      _processDeleteRequest(request, location_info);
      break;
    default:
      break;
  }
}

HttpResponse::HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info)
    : _sendingState(HTTP_SENDING_HEADER), _bodyFd(-1), _bodySent(0), _headerSent(0), _statusCode(0), _contentLength(0) {
  if (cgi_response.isExecuteError()) {
    _makeErrorResponse(500, cgi_response, location_info);
  }

  if (cgi_response.httpVersion() != "HTTP/1.1") {
    _makeErrorResponse(505, cgi_response, location_info);
    return;
  }
  _httpVersion   = cgi_response.httpVersion();
  _statusCode    = cgi_response.statusCode();
  _message       = cgi_response.statusMessage();
  _contentLength = cgi_response.bodySize();
  _contentType   = cgi_response.contentType();

  _bodyFd = cgi_response.bodyFd();
  _makeHeader();
}

// Interface

void HttpResponse::sendResponse(int fd) {
  int sent_size;
  int read_size;

  switch (_sendingState) {
    case HTTP_SENDING_HEADER:
      _sendChunk(fd, _header.c_str(), _header.size(), _headerSent);
      if (!_sendingStateTransition(_header.size(), _headerSent, HTTP_SENDING_HEADER)) {
        break;
      }

    case HTTP_SENDING_TEMPBODY:
      _sendChunk(fd, _tempBody.data(), _tempBody.size(), _bodySent);
      if (!_sendingStateTransition(_tempBody.size(), _bodySent, HTTP_SENDING_TEMPBODY)) {
        break;
      }
      if (_bodyFd == -1) {
        _sendingState = HTTP_SENDING_DONE;
        break;
      }

    case HTTP_SENDING_FILEBODY:
      read_size = read(_bodyFd, GetLine::publicBuffer, PUBLIC_BUFFER_SIZE);
      sent_size = send(fd, GetLine::publicBuffer, read_size, 0);
      if (sent_size == -1 || read_size == -1) {
        _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
        Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
        return;
      }
      ft::syscall::lseek(_bodyFd, static_cast<off_t>((read_size - sent_size) * -1), SEEK_CUR);
      _bodySent += sent_size;
      if (!_sendingStateTransition(_contentLength, _bodySent, HTTP_SENDING_FILEBODY)) {
        break;
      }

    default:
      _sendingState = HTTP_SENDING_DONE;
      close(_bodyFd);  // ?
      break;
  }
}

// Method
template <typename T>
void HttpResponse::_sendChunk(int fd, const T* base, size_t total_size, size_t& already_sent) {
  const T* pos       = base + already_sent;
  int      remains   = total_size - already_sent;
  int      sent_size = send(fd, pos, remains, 0);
  if (sent_size == -1) {
    _sendingState = HTTP_SENDING_CONNECTION_CLOSED;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return;
  }
  already_sent += sent_size;
}

bool HttpResponse::_sendingStateTransition(size_t total_size, size_t already_sent,
                                           HttpResponseSendingState current_state) {
  if (already_sent != total_size) {
    _sendingState = current_state;
    return false;
  }
  return true;
}

void HttpResponse::_makeHeader() {
  std::stringstream response_stream;

  response_stream << _httpVersion << ' ' << _statusCode << ' ' << _message << "\r\n";

  response_stream << "Content-Length: " << _contentLength << "\r\n";
  if (!_contentType.empty()) {
    response_stream << "Content-Type: " << _contentType << "\r\n";
  }
  if (!_location.empty()) {
    response_stream << "Location: " << _location << "\r\n";
  }
  response_stream << "\r\n";
  _header = response_stream.str();
}

void HttpResponse::_processGetRequest(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isFileExist() && file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      _makeAutoIndexResponse(request, location_info, file_manager);
      return;
    } else if (index_page.empty()) {
      file_manager.appendToPath("index.html");
    } else {
      file_manager.appendToPath(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);
    return;
  }

  _bodyFd        = open(file_manager.filePath().c_str(), O_RDONLY);
  _contentLength = lseek(_bodyFd, 0, SEEK_END);  // error -> throw;
  lseek(_bodyFd, 0, SEEK_SET);
  if (_contentLength > static_cast<size_t>(location_info.maxBodySize)) {
    _makeErrorResponse(413, request, location_info);
  }

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);
  _contentType = _getContentType(file_manager.filePath());
  Log::log()(LOG_LOCATION, "Get request processed.");
  _makeHeader();
}

void HttpResponse::_makeAutoIndexResponse(HttpRequest& request, LocationInfo& location_info,
                                          FileManager& file_manager) {
  std::string body;

  file_manager.openDirectoy();

  body += "<html><head><title> Index of / " + file_manager.filePath() + " / </title></head>\n";
  body += "<body><h1> Index of / " + file_manager.filePath() + " / </h1><hr>\n";

  std::string file_name = file_manager.readDirectoryEntry();
  while (!file_name.empty()) {
    if (file_name != ".") {
      body += "<pre><a href = \"";
      body += file_manager.filePath().substr(location_info.root.size() + 1) + '/' + file_name + "\">" + file_name +
              "/</a></pre>\n";
    }
    file_name = file_manager.readDirectoryEntry();
  }
  body += "<hr></body></html>";

  _tempBody.insert(_tempBody.begin(), body.c_str(), body.c_str() + body.size());

  _httpVersion   = request.httpVersion();
  _statusCode    = 200;
  _message       = _getMessage(_statusCode);
  _contentLength = _tempBody.size();

  _makeHeader();
}

void HttpResponse::_processHeadRequest(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isFileExist() && file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      _makeAutoIndexResponse(request, location_info, file_manager);
      return;
    } else if (index_page.empty()) {
      file_manager.appendToPath("index.html");
    } else {
      file_manager.appendToPath(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);
    return;
  }

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);
  _contentType = _getContentType(file_manager.filePath());
  Log::log()(LOG_LOCATION, "Head request processed.");

  _makeHeader();
}

void HttpResponse::_processPostRequest(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);

  if (file_manager.isFileExist()) {
    std::string file_name = request.uri().substr(location_info.id.size());
    _makeRedirResponse(303, request, location_info, file_name);
    return;
  }

  _bodyFd = request.bodyFd();
  // event.toSendFd = open(file_manager.filePath(), O_RDONLY);

  // file_manager.openOutFile();
  // file_manager.outFile().write(reinterpret_cast<const char*>(request.body().data()), request.body().size());  // ?
  // file_manager.outFile().close();

  _httpVersion = request.httpVersion();
  _statusCode  = 201;
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Post request processed.");
  _makeHeader();
}

void HttpResponse::_processPutRequest(HttpRequest& request, LocationInfo& location_info) {
  if (request.body().size() == 0) {
    _makeErrorResponse(402, request, location_info);
    return;
  }
  if (static_cast<int>(request.body().size()) > location_info.maxBodySize) {
    _makeErrorResponse(413, request, location_info);
    return;
  }

  FileManager file_manager(request.uri(), location_info);

  if (file_manager.isFileExist()) {
    file_manager.openOutFile(std::ofstream::trunc);
    file_manager.outFile().write(reinterpret_cast<const char*>(request.body().data()), request.body().size());
    _statusCode = 200;
  } else {
    file_manager.openOutFile();
    file_manager.outFile().write(reinterpret_cast<const char*>(request.body().data()), request.body().size());
    _statusCode = 201;
  }
  _httpVersion = request.httpVersion();
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Put request processed.");
  _makeHeader();
}

void HttpResponse::_processDeleteRequest(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);
    return;
  }

  file_manager.removeFile();

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);

  Log::log()(LOG_LOCATION, "Delete request processed.");
  _makeHeader();
}

void HttpResponse::_makeErrorResponse(int error_code, Request& request, LocationInfo& location_info) {
  int file_fd = -1;
  if (location_info.defaultErrorPages.count(error_code)) {
    std::string file_name = location_info.root + location_info.defaultErrorPages[error_code];
    file_fd               = open(file_name.c_str(), O_RDONLY);
    Log::log()(true, "file open STATUS", file_fd, INFILE);
    Log::log()(true, "file name", file_name, INFILE);
  }

  if (file_fd == -1) {
    std::stringstream default_error_page;
    default_error_page << DEFAULT_ERROR_PAGE_DIR << '/' << error_code << ".html";
    file_fd = open(default_error_page.str().c_str(), O_RDONLY);
    Log::log()(true, "file open STATUS", file_fd, INFILE);
    Log::log()(true, "file name", default_error_page.str(), INFILE);
  }

  if (request.method() != HEAD) {
    _bodyFd = file_fd;
  }

  _httpVersion = request.httpVersion();
  _statusCode  = error_code;
  _message     = _getMessage(_statusCode);
  if (_bodyFd != -1) {
    _contentLength = lseek(_bodyFd, 0, SEEK_END);
    lseek(_bodyFd, 0, SEEK_SET);
  }
  Log::log()(LOG_LOCATION, "Error page returned.");
  _makeHeader();
}

void HttpResponse::_makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info,
                                      const std::string& location_field) {
  _httpVersion = request.httpVersion();
  _statusCode  = redir_code;
  _message     = _getMessage(_statusCode);

  if (location_field.empty()) {
    if (location_info.redirPath.empty()) {
      _location = "index.html";
    } else {
      _location = location_info.redirPath;
    }
  } else {
    _location = location_field;
  }

  // add other field ?
  Log::log()(LOG_LOCATION, "Redirection address returned.");
  _makeHeader();
}

bool HttpResponse::_isAllowedMethod(int method, std::vector<std::string>& allowed_methods) {
  bool is_existed;
  switch (method) {
    case GET:
      is_existed = std::find(allowed_methods.begin(), allowed_methods.end(), "GET") != allowed_methods.end();
      return is_existed;
    case POST:
      is_existed = std::find(allowed_methods.begin(), allowed_methods.end(), "POST") != allowed_methods.end();
      return is_existed;
    case HEAD:
      is_existed = std::find(allowed_methods.begin(), allowed_methods.end(), "HEAD") != allowed_methods.end();
      return is_existed;
    case PUT:
      is_existed = std::find(allowed_methods.begin(), allowed_methods.end(), "PUT") != allowed_methods.end();
      return is_existed;
    case DELETE:
      is_existed = std::find(allowed_methods.begin(), allowed_methods.end(), "DELETE") != allowed_methods.end();
      return is_existed;
    default:
      return false;
  }
}

std::string HttpResponse::_getMessage(int status_code) {
  switch (status_code) {
    case 100:
      return "Continue";  // ?
    case 101:
      return "Switching Protocols";  // ?
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 202:
      return "Accepted";
    case 204:
      return "No Content";  // ?
    case 300:
      return "Multiple Choices";  //?
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";  // ?
    case 303:
      return "See Other";
    case 304:
      return "Not Modified";  // ?
    case 305:
      return "Use Proxy";  // ?
    case 307:
      return "Temporary Redirect";  // ?
    case 400:
      return "Bad Request";
    case 402:
      return "Payment Required";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 413:
      return "Payload Too Large";
    case 500:
      return "Internal Server Error";  // ?
    case 501:
      return "Not Implemented";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";  // ?
    case 504:
      return "Gateway Timeout";
    case 505:
      return "HTTP Version Not Supported";
    default:
      return "default";
  }
}

std::string HttpResponse::_getContentType(const std::string& file_name) {
  std::string::size_type ext_delim = file_name.rfind('.');
  std::string            ext       = file_name.substr(ext_delim + 1);

  if (ext == "html" || ext == "htm" || ext == "shtml") {
    return "text/html";
  } else if (ext == "xml" || ext == "rss") {
    return "text/xml";
  } else if (ext == "css") {
    return "text/css";
  } else if (ext == "js") {
    return "application/x-javasciprt";
  } else if (ext == "gif") {
    return "image/gif";
  } else if (ext == "jpg" | ext == "jpeg") {
    return "image/jpeg";
  } else if (ext == "png") {
    return "image/png";
  } else if (ext == "ico") {
    return "image/x-icon";
  } else if (ext == "mp3") {
    return "audio/mpeg";
  }
  return "text/plain";
}
