#pragma once

#include "HttpResponse.hpp"
#include "Request.hpp"

class HttpRequest;
class CgiResponse;
class HttpResponse;

class ResponseFactory {
 private:
  static std::string                 _httpVersion;
  static enum HttpResponseStatusCode _statusCode;
  static std::string                 _message;
  static size_t                      _contentLength;
  static std::string                 _contentType;
  static std::string                 _location;

 private:
  static HttpResponse* _getResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* _headResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* _autoIndexResponse(HttpRequest& request, LocationInfo& location_info, FileManager& file_manager);
  static HttpResponse* _postResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* _putResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* _deleteResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* _redirResponse(HttpResponseStatusCode redir_code, HttpRequest& request,
                                      LocationInfo& location_info, const std::string& location_field = "");

  static bool        _isAllowedMethod(MethodType method, std::vector<std::string>& allowed_methods);
  static std::string _getMessage(HttpResponseStatusCode status_code);
  static std::string _getContentType(const std::string& file_name);
  static std::string _makeHeader();
  static void        _initialize();

 public:
  static HttpResponse* makeResponse(HttpRequest& request, LocationInfo& location_info);
  static HttpResponse* makeResponse(CgiResponse& request, LocationInfo& location_info);
  static HttpResponse* errorResponse(HttpResponseStatusCode error_code, Request& request, LocationInfo& location_info);
};
