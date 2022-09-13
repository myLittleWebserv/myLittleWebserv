#include "ResponseFactory.hpp"

#include <fcntl.h>

#include <sstream>

#include "CgiResponse.hpp"
#include "Config.hpp"
#include "FileManager.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Log.hpp"
#include "syscall.hpp"
std::string                 ResponseFactory::_httpVersion;
enum HttpResponseStatusCode ResponseFactory::_statusCode;
std::string                 ResponseFactory::_message;
size_t                      ResponseFactory::_contentLength;
std::string                 ResponseFactory::_contentType;
std::string                 ResponseFactory::_location;

HttpResponse* ResponseFactory::makeResponse(HttpRequest& request, LocationInfo& location_info) {
  _initialize();
  if (request.isBadRequest()) {
    return errorResponse(STATUS_BAD_REQUEST, request, location_info);
  }
  if (request.httpVersion() != "HTTP/1.1") {
    return errorResponse(STATUS_HTTP_VERSION_NOT_SUPPORTED, request, location_info);
  }
  if (request.method() == NOT_IMPL) {
    return errorResponse(STATUS_NOT_IMPLEMENTED, request, location_info);
  }
  if (!_isAllowedMethod(request.method(), location_info.allowedMethods)) {
    return errorResponse(STATUS_METHOD_NOT_ALLOWED, request, location_info);
  }

  if (location_info.redirStatus != -1) {
    return _redirResponse(static_cast<HttpResponseStatusCode>(location_info.redirStatus), request, location_info);
  }

  switch (request.method()) {
    case GET:
      return _getResponse(request, location_info);
    case HEAD:
      return _headResponse(request, location_info);
    case POST:
      return _postResponse(request, location_info);
    case PUT:
      return _putResponse(request, location_info);
    case DELETE:
      return _deleteResponse(request, location_info);
    case NOT_IMPL:
      return errorResponse(STATUS_NOT_IMPLEMENTED, request, location_info);
  }
}

HttpResponse* ResponseFactory::makeResponse(CgiResponse& cgi_response, LocationInfo& location_info) {
  _initialize();
  if (cgi_response.isExecError()) {
    return errorResponse(STATUS_INTERNAL_SERVER_ERROR, cgi_response, location_info);
  }

  if (cgi_response.httpVersion() != "HTTP/1.1") {
    return errorResponse(STATUS_HTTP_VERSION_NOT_SUPPORTED, cgi_response, location_info);
  }
  _httpVersion   = cgi_response.httpVersion();
  _statusCode    = static_cast<HttpResponseStatusCode>(cgi_response.statusCode());  // ?
  _message       = cgi_response.statusMessage();
  _contentLength = cgi_response.bodySize();
  _contentType   = cgi_response.contentType();

  Log::log()(true, "_contentLengh.make_cgires", _contentLength);
  return new HttpResponse(cgi_response.storage(), _makeHeader(), _statusCode, _contentLength, cgi_response.bodyFd());
}

HttpResponse* ResponseFactory::_getResponse(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isFileExist() && file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      return _autoIndexResponse(request, location_info, file_manager);
    } else if (index_page.empty()) {
      file_manager.appendToPath("index.html");
    } else {
      file_manager.appendToPath(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    return errorResponse(STATUS_NOT_FOUND, request, location_info);
  }

  int fd         = open(file_manager.filePath().c_str(), O_RDONLY | O_NONBLOCK);
  _contentLength = ft::syscall::lseek(fd, 0, SEEK_END);  // error -> throw;
  ft::syscall::lseek(fd, 0, SEEK_SET);
  if (_contentLength > static_cast<size_t>(location_info.maxBodySize)) {
    return errorResponse(STATUS_PAYLOAD_TOO_LARGE, request, location_info);
  }

  _httpVersion = request.httpVersion();
  _statusCode  = STATUS_OK;
  _message     = _getMessage(_statusCode);
  _contentType = _getContentType(file_manager.filePath());
  Log::log()(LOG_LOCATION, "Get request processed.");
  return new HttpResponse(_makeHeader(), _statusCode, _contentLength, fd);
}

HttpResponse* ResponseFactory::_headResponse(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isFileExist() && file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      return _autoIndexResponse(request, location_info, file_manager);
    } else if (index_page.empty()) {
      file_manager.appendToPath("index.html");
    } else {
      file_manager.appendToPath(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    return errorResponse(STATUS_NOT_FOUND, request, location_info);
  }

  _httpVersion = request.httpVersion();
  _statusCode  = STATUS_OK;
  _message     = _getMessage(_statusCode);
  _contentType = _getContentType(file_manager.filePath());
  Log::log()(LOG_LOCATION, "Get request processed.");
  return new HttpResponse(_makeHeader(), _statusCode, 0);
}

HttpResponse* ResponseFactory::_autoIndexResponse(HttpRequest& request, LocationInfo& location_info,
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

  _httpVersion   = request.httpVersion();
  _statusCode    = STATUS_OK;
  _message       = _getMessage(_statusCode);
  _contentLength = body.size();

  return new HttpResponse(_makeHeader() + body, _statusCode);
}

HttpResponse* ResponseFactory::_postResponse(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  int         fd;

  // if (request.body().size() == 0) {
  //   _makeErrorResponse(402, request, location_info);
  //   return;
  //

  if (file_manager.isFileExist()) {
    std::string file_name = request.uri().substr(location_info.id.size());
    return _redirResponse(STATUS_MOVED_PERMANENTLY, request, location_info, file_name);
  }

  if (file_manager.isConflict()) {
    return errorResponse(STATUS_CONFLICT, request, location_info);
  }

  fd = ft::syscall::open(file_manager.filePath().c_str(), O_WRONLY | O_CREAT | O_NONBLOCK);

  _httpVersion = request.httpVersion();
  _statusCode  = STATUS_CREATED;
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Post request processed.");
  return new HttpResponse(_makeHeader(), _statusCode, 0, fd);
}

HttpResponse* ResponseFactory::_putResponse(HttpRequest& request, LocationInfo& location_info) {
  // if (request.body().size() == 0) {
  //   _makeErrorResponse(402, request, location_info);
  //   return;
  //

  FileManager file_manager(request.uri(), location_info);
  int         fd;

  if (file_manager.isFileExist()) {
    fd          = ft::syscall::open(file_manager.filePath().c_str(), O_WRONLY | O_TRUNC | O_NONBLOCK);
    _statusCode = STATUS_OK;
  } else if (!file_manager.isConflict()) {
    fd          = ft::syscall::open(file_manager.filePath().c_str(), O_WRONLY | O_CREAT | O_NONBLOCK);
    _statusCode = STATUS_CREATED;
  } else {
    return errorResponse(STATUS_CONFLICT, request, location_info);
  }

  Log::log()(true, "put file fd", fd);

  _httpVersion = request.httpVersion();
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Put request processed.");
  return new HttpResponse(_makeHeader(), _statusCode, 0, fd);
}

HttpResponse* ResponseFactory::_deleteResponse(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);

  if (!file_manager.isFileExist()) {
    return errorResponse(STATUS_NOT_FOUND, request, location_info);
  }

  file_manager.removeFile();

  _httpVersion = request.httpVersion();
  _statusCode  = STATUS_OK;
  _message     = _getMessage(_statusCode);

  Log::log()(LOG_LOCATION, "Delete request processed.");
  return new HttpResponse(_makeHeader(), _statusCode);
}

HttpResponse* ResponseFactory::errorResponse(HttpResponseStatusCode error_code, Request& request,
                                             LocationInfo& location_info) {
  _initialize();
  int               fd = -1;
  std::string       file_name;
  std::stringstream default_error_page;

  if (location_info.defaultErrorPages.count(error_code)) {
    FileManager file_manager(location_info.root);
    file_manager.appendToPath(location_info.defaultErrorPages[error_code]);
    fd = ::open(file_manager.filePath().c_str(), O_RDONLY | O_NONBLOCK);
    Log::log()(true, "file open STATUS", fd, INFILE);
    Log::log()(true, "file name", file_name, INFILE);
  }

  if (fd == -1) {
    default_error_page << DEFAULT_ERROR_PAGE_DIR << '/' << error_code << ".html";
    fd = ft::syscall::open(default_error_page.str().c_str(), O_RDONLY | O_NONBLOCK);
    Log::log()(true, "file open STATUS", fd, INFILE);
    Log::log()(true, "file name", default_error_page.str(), INFILE);
  }

  if (request.method() == HEAD) {
    ft::syscall::close(fd);
    fd = -1;
  }

  _httpVersion = request.httpVersion();
  _statusCode  = error_code;
  _message     = _getMessage(_statusCode);
  if (fd != -1) {
    _contentLength = ft::syscall::lseek(fd, 0, SEEK_END);
    ft::syscall::lseek(fd, 0, SEEK_SET);
  }
  Log::log()(LOG_LOCATION, "Error page returned.");
  return new HttpResponse(_makeHeader(), _statusCode, _contentLength, fd);
}

HttpResponse* ResponseFactory::_redirResponse(HttpResponseStatusCode redir_code, HttpRequest& request,
                                              LocationInfo& location_info, const std::string& location_field) {
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

  Log::log()(LOG_LOCATION, "Redirection address returned.");
  return new HttpResponse(_makeHeader(), _statusCode);
}

bool ResponseFactory::_isAllowedMethod(MethodType method, std::vector<std::string>& allowed_methods) {
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

std::string ResponseFactory::_getMessage(HttpResponseStatusCode status_code) {
  switch (status_code) {
    case STATUS_CONTINUE:
      return "Continue";  // ?
    case STATUS_SWITCHING_PROTOCOLS:
      return "Switching Protocols";  // ?
    case STATUS_OK:
      return "OK";
    case STATUS_CREATED:
      return "Created";
    case STATUS_ACCEPTED:
      return "Accepted";
    case STATUS_NO_CONTENT:
      return "No Content";  // ?
    case STATUS_MULTIPLE_CHOICES:
      return "Multiple Choices";  //?
    case STATUS_MOVED_PERMANENTLY:
      return "Moved Permanently";
    case STATUS_SEE_OTHER:
      return "See Other";
    case STATUS_BAD_REQUEST:
      return "Bad Request";
    case STATUS_PAYMENT_REQUIRED:
      return "Payment Required";
    case STATUS_NOT_FOUND:
      return "Not Found";
    case STATUS_METHOD_NOT_ALLOWED:
      return "Method Not Allowed";
    case STATUS_CONFLICT:
      return "Conflict";
    case STATUS_PAYLOAD_TOO_LARGE:
      return "Payload Too Large";
    case STATUS_INTERNAL_SERVER_ERROR:
      return "Internal Server Error";  // ?
    case STATUS_NOT_IMPLEMENTED:
      return "Not Implemented";
    case STATUS_BAD_GATEWAY:
      return "Bad Gateway";
    case STATUS_SERVICE_UNAVAILABLE:
      return "Service Unavailable";  // ?
    case STATUS_GATEWAY_TIMEOUT:
      return "Gateway Timeout";
    case STATUS_HTTP_VERSION_NOT_SUPPORTED:
      return "HTTP Version Not Supported";
    default:
      return "default";
  }
}

std::string ResponseFactory::_getContentType(const std::string& file_name) {
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

std::string ResponseFactory::_makeHeader() {
  std::stringstream response_stream;

  response_stream << _httpVersion << ' ' << static_cast<int>(_statusCode) << ' ' << _message << "\r\n";

  response_stream << "Content-Length: " << _contentLength << "\r\n";
  if (!_contentType.empty()) {
    response_stream << "Content-Type: " << _contentType << "\r\n";
  }
  if (!_location.empty()) {
    response_stream << "Location: " << _location << "\r\n";
  }
  response_stream << "\r\n";
  // _initialize();
  return response_stream.str();
}

void ResponseFactory::_initialize() {
  _httpVersion   = "";
  _statusCode    = STATUS_OK;
  _message       = "";
  _contentLength = 0;
  _contentType   = "";
  _location      = "";
}