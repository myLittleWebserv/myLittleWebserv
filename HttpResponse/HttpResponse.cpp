#include "HttpResponse.hpp"

#include <algorithm>
#include <fstream>
#include <istream>
#include <sstream>

#include "Config.hpp"
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
  if (!_allowedMethod(request.method(), location_info.allowedMethods)) {
    _makeErrorResponse(405, request, location_info);
    return;
  }

  switch (request.method()) {
    case GET:
      _processGetRequest(request, location_info);
      break;
    case POST:
      _processPostRequest(request, location_info);
      break;
    default:
      break;
  }
}

HttpResponse::HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info) {}

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
  response_stream << "\r\n";
  return response_stream.str();
}

// Method

void HttpResponse::_processGetRequest(HttpRequest& request, LocationInfo& location_info) {
  if (location_info.redirStatus != -1) {
    _makeRedirResponse(location_info.redirStatus, request, location_info);
    return;
  }

  std::string   file_name = location_info.root + request.uri();
  std::ifstream file(file_name);

  if (!file.is_open()) {
    _makeErrorResponse(404, request, location_info);  // 403 ?
    return;
  }

  _httpVersion = request.httpVersion();
  _statusCode  = 200;
  _message     = _getMessage(_statusCode);
  _body.insert(_body.begin(), std::istream_iterator<unsigned char>(file), std::istream_iterator<unsigned char>());
  _contentLength = _body.size();
  _contentType   = _getContentType(file_name);
}

void HttpResponse::_processPostRequest(HttpRequest& request, LocationInfo& location_info) {
  if (location_info.redirStatus != -1) {
    _makeRedirResponse(location_info.redirStatus, request, location_info);
    return;
  }

  std::string   file_name = location_info.root + request.uri();
  std::ifstream ifile(file_name);

  if (ifile.is_open()) {
    _makeRedirResponse(303, request, location_info);
    return;
  }

  std::ofstream ofile(file_name);
  ofile.write(reinterpret_cast<const char*>(request.body().data()), request.body().size());

  _httpVersion = request.httpVersion();
  _statusCode  = 201;
  _message     = _getMessage(_statusCode);
}

void HttpResponse::_makeErrorResponse(int error_code, HttpRequest& request, LocationInfo& location_info) {
  _httpVersion = request.httpVersion();
  _statusCode  = error_code;
  _message     = _getMessage(_statusCode);

  // add error page to body
}

void HttpResponse::_makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info) {
  _httpVersion = request.httpVersion();
  _statusCode  = redir_code;
  _message     = _getMessage(_statusCode);

  // add other field ?
}

bool HttpResponse::_allowedMethod(int method, std::vector<std::string>& allowed_methods) {
  bool is_existed;
  for (std::vector<std::string>::iterator it = allowed_methods.begin(); it != allowed_methods.end(); ++it) {
    std::cout << *it << ' ';
  }
  std::cout << std::endl;
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
      return "Continue";
    case 101:
      return "Switching Protocols";
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 202:
      return "Accepted";
    case 204:
      return "No Content";
    case 300:
      return "Multiple Choices";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 303:
      return "See Other";
    case 304:
      return "Not Modified";
    case 305:
      return "Use Proxy";
    case 307:
      return "Temporary Redirect";
    case 400:
      return "Bad Request";
    case 402:
      return "Payment Required";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    case 502:
      return "Bad Gateway";
    case 503:
      return "Service Unavailable";
    case 504:
      return "Gateway Timeout";
    case 505:
      return "HTTP Version Not Supported";
    default:
      return "default";
  }
}

std::string HttpResponse::_getContentType(const std::string& file_name) { return "plain/text"; }