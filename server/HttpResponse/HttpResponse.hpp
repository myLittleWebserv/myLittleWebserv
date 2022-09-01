#ifndef HTTPRESPONSE_HPP

#include <fstream>
#include <string>
#include <vector>

#define DEFAULT_ERROR_PAGE_DIR "error_pages"

#include "Storage.hpp"

class HttpRequest;
class CgiResponse;
class FileManager;
struct LocationInfo;

class HttpResponse {
  // Member Variable
 private:
  std::vector<unsigned char> _body;
  Storage                    _storage;
  std::string                _httpVersion;
  int                        _statusCode;
  std::string                _message;
  std::string                _location;
  int                        _contentLength;
  std::string                _contentType;  // default = plain/text;

  // Method
 private:
  void        _responseToStorage();
  void        _headerToStorage();
  void        _fileToBody(std::ifstream& file);
  void        _processGetRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processHeadRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processPostRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processPutRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processDeleteRequest(HttpRequest& request, LocationInfo& location_info);
  void        _makeAutoIndexResponse(HttpRequest& request, LocationInfo& location_info, FileManager& file_manager);
  void        _makeErrorResponse(int error_code, HttpRequest& request, LocationInfo& location_info);
  void        _makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info,
                                 const std::string& location_field = "");
  bool        _isAllowedMethod(int method, std::vector<std::string>& allowed_method);
  std::string _getMessage(int status_code);
  std::string _getContentType(const std::string& file_name);

  // Constructor
 public:
  HttpResponse(HttpRequest& request, LocationInfo& location_info);
  HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info);
  // Interface
 public:
  Storage& storage() { return _storage; }
};

#endif
