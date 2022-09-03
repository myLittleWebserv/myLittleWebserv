#include "HttpResponse.hpp"

#include <dirent.h>

#include <algorithm>
#include <sstream>

#include "CgiResponse.hpp"
#include "Config.hpp"
#include "FileManager.hpp"
#include "HttpRequest.hpp"

// Constructor

HttpResponse::HttpResponse(HttpRequest& request, LocationInfo& location_info) : _statusCode(0), _contentLength(0) {
  if (request.isBadRequest()) {
    _makeErrorResponse(400, request, location_info);
    return;
  }
  if (request.httpVersion() != "HTTP/1.1") {  // 더 아래 버전 ?
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

HttpResponse::HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info) {
  if (cgi_response.isError()) {
    _makeErrorResponse(500, cgi_response, location_info);
  }

  if (cgi_response.httpVersion() != "HTTP/1.1") {
    _makeErrorResponse(505, cgi_response, location_info);
    return;
  }

  _body.insert(_body.end(), cgi_response.body().begin(), cgi_response.body().end());

  _httpVersion   = cgi_response.httpVersion();
  _statusCode    = cgi_response.statusCode();
  _message       = cgi_response.statusMessage();
  _contentLength = _body.size();
  _contentType   = cgi_response.contentType();
  _responseToStorage();
}

// Interface

std::string HttpResponse::headerToString() {
  std::stringstream response_stream;

  response_stream << _httpVersion << ' ' << _statusCode << ' ' << _message << "\r\n";

  if (_contentLength != 0) {
    response_stream << "Content-Legnth: " << _contentLength << "\r\n";
  }
  if (!_contentType.empty()) {
    response_stream << "Content-Type: " << _contentType << "\r\n";
  }
  if (!_location.empty()) {
    response_stream << "Location: " << _location << "\r\n";
  }
  response_stream << "\r\n";
  return response_stream.str();
}

// Method
void HttpResponse::_headerToStorage() {
  std::stringstream response_stream;

  response_stream << _httpVersion << ' ' << _statusCode << ' ' << _message << "\r\n";

  if (_contentLength != 0) {
    response_stream << "Content-Legnth: " << _contentLength << "\r\n";
  }
  if (!_contentType.empty()) {
    response_stream << "Content-Type: " << _contentType << "\r\n";
  }
  if (!_location.empty()) {
    response_stream << "Location: " << _location << "\r\n";
  }
  response_stream << "\r\n";
  std::string response_header = response_stream.str();
  _storage.insert(_storage.end(), response_header.begin(), response_header.end());
}

void HttpResponse::_responseToStorage() {
  _headerToStorage();
  _storage.insert(_storage.end(), _body.begin(), _body.end());
}

void HttpResponse::_fileToBody(std::ifstream& file) {
  while (!file.eof()) {
    std::string line;
    std::getline(file, line);
    _body.insert(_body.end(), line.c_str(), line.c_str() + line.size());
    _body.insert(_body.end(), '\n');
  }
}

void HttpResponse::_processGetRequest(HttpRequest& request, LocationInfo& location_info) {
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      _makeAutoIndexResponse(request, location_info, file_manager);
      return;
    } else if (index_page.empty()) {
      _makeRedirResponse(301, request, location_info);
      return;
    } else {
      file_manager.addIndexToName(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);  // ? 403
    return;
  }

  file_manager.openInFile();
  _fileToBody(file_manager.inFile());
  if (static_cast<int>(_body.size()) > location_info.maxBodySize) {
    _makeErrorResponse(413, request, location_info);
  }

  _httpVersion   = request.httpVersion();
  _statusCode    = 200;
  _message       = _getMessage(_statusCode);
  _contentLength = _body.size();
  _contentType   = _getContentType(file_manager.fileName());
  Log::log()(LOG_LOCATION, "Get request processed.");
  _responseToStorage();
}

void HttpResponse::_makeAutoIndexResponse(HttpRequest& request, LocationInfo& location_info,
                                          FileManager& file_manager) {
  std::string body;

  (void)location_info;

  file_manager.openDirectoy();

  body += "<html><head><title> Index of / " + file_manager.fileName() + " / </title></head>\n";
  body += "<body><h1> Index of / " + file_manager.fileName() + " / </h1><hr>\n";

  std::string file_name = file_manager.readDirectoryEntry();
  while (!file_name.empty()) {
    if (file_name != ".") {
      body += "<pre><a href = \"";
      body += file_name + "\">" + file_name + "/</a></pre>\n";
    }
    file_name = file_manager.readDirectoryEntry();
  }
  body += "<hr></body></html>";

  _body.insert(_body.begin(), body.c_str(), body.c_str() + body.size());

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);

  _responseToStorage();
}

void HttpResponse::_processHeadRequest(HttpRequest& request, LocationInfo& location_info) {  // ?
  FileManager file_manager(request.uri(), location_info);
  bool        isAutoIndexOn = location_info.isAutoIndexOn;
  std::string index_page    = location_info.indexPagePath;

  if (file_manager.isDirectory()) {
    if (index_page.empty() && isAutoIndexOn) {
      _makeAutoIndexResponse(request, location_info, file_manager);
      return;
    } else if (index_page.empty()) {
      _makeRedirResponse(301, request, location_info);
      return;
    } else {
      file_manager.addIndexToName(location_info.indexPagePath);
    }
  }

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);  // ? 403
    return;
  }

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);
  _contentType = _getContentType(file_manager.fileName());
  Log::log()(LOG_LOCATION, "Head request processed.");
  _headerToStorage();
}

void HttpResponse::_processPostRequest(HttpRequest& request, LocationInfo& location_info) {
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
    _makeRedirResponse(303, request, location_info, file_manager.fileName());
    return;
  }

  file_manager.openOutFile();
  file_manager.outFile().write(reinterpret_cast<const char*>(request.body().data()), request.body().size());

  _httpVersion = request.httpVersion();
  _statusCode  = 201;
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Post request processed.");
  _responseToStorage();
}

void HttpResponse::_processPutRequest(HttpRequest& request, LocationInfo& location_info) {  // ?
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
  _responseToStorage();
}

void HttpResponse::_processDeleteRequest(HttpRequest& request, LocationInfo& location_info) {  // ?
  FileManager file_manager(request.uri(), location_info);

  if (!file_manager.isFileExist()) {
    _makeErrorResponse(404, request, location_info);
    return;
  }

  file_manager.removeFile();

  Log::log()(LOG_LOCATION, "Delete request processed.");
  _responseToStorage();
}

void HttpResponse::_makeErrorResponse(int error_code, Request& request, LocationInfo& location_info) {
  std::ifstream file;

  if (location_info.defaultErrorPages.count(error_code)) {
    std::stringstream file_name;
    file_name << location_info.root << '/' << location_info.defaultErrorPages[error_code];
    file.open(file_name.str().c_str());
  }

  if (!file.is_open()) {
    std::stringstream default_error_page;
    default_error_page << DEFAULT_ERROR_PAGE_DIR << '/' << error_code << ".html";
    file.open(default_error_page.str().c_str());
  }

  if (request.method() != HEAD) {
    _fileToBody(file);
  }

  _httpVersion = request.httpVersion();
  _statusCode  = error_code;
  _message     = _getMessage(_statusCode);
  Log::log()(LOG_LOCATION, "Error page returned.");

  _responseToStorage();
}

void HttpResponse::_makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info,
                                      const std::string& location_field) {
  _httpVersion = request.httpVersion();
  _statusCode  = redir_code;
  _message     = _getMessage(_statusCode);

  if (location_field.empty()) {
    if (location_info.redirPath.empty()) {
      _location = "index.html";  // ?
    } else {
      _location = location_info.redirPath;
    }
  } else {
    _location = location_field;
  }

  // add other field ?
  Log::log()(LOG_LOCATION, "Redirection address returned.");
  _responseToStorage();
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
