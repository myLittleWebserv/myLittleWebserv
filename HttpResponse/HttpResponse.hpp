#ifndef HTTPRESPONSE_HPP

#include <string>
#include <vector>

#define DEFAULT_ERROR_PAGE_DIR "ErrorPages"

class HttpRequest;
class CgiResponse;
struct LocationInfo;

class HttpResponse {
  // Member Variable
 private:
  std::vector<unsigned char> _body;
  std::string                _httpVersion;
  int                        _statusCode;
  std::string                _message;
  std::string                _location;
  int                        _contentLength;
  std::string                _contentType;  // default = plain/text;

  // Method
 private:
  void        _processGetRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processHeadRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processPostRequest(HttpRequest& request, LocationInfo& location_info);
  void        _makeErrorResponse(int error_code, HttpRequest& request, LocationInfo& location_info);
  void        _makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info,
                                 const std::string& location_field = "");
  bool        _allowedMethod(int method, std::vector<std::string>& allowed_method);
  std::string _getMessage(int status_code);
  std::string _getContentType(const std::string& file_name);

  // Constructor
 public:
  HttpResponse(HttpRequest& request, LocationInfo& location_info);
  HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info);
  // Interface
 public:
  std::string                       headerToString();
  const std::vector<unsigned char>& body() { return _body; }
};

#endif
