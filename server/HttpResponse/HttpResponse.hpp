#ifndef HTTPRESPONSE_HPP

#include <fstream>
#include <string>
#include <vector>

#define DEFAULT_ERROR_PAGE_DIR "error_pages"

#include "Storage.hpp"

class Request;
class HttpRequest;
class CgiResponse;
class FileManager;
struct LocationInfo;

enum HttpResponseSendingState { HTTP_SENDING_HEADER, HTTP_SENDING_BODY, HTTP_SENDING_DONE };

class HttpResponse {
  // Types
  typedef std::vector<unsigned char> vector;

  // Member Variable
 private:
  enum HttpResponseSendingState _sendingState;
  vector                        _tempBody;
  vector::pointer               _body;
  size_t                        _bodySent;
  size_t                        _headerSent;
  std::string                   _header;
  std::string                   _httpVersion;
  int                           _statusCode;
  std::string                   _message;
  std::string                   _location;
  size_t                        _contentLength;
  std::string                   _contentType;  // default = plain/text;

  // Method
 private:
  void        _makeHeader();
  void        _fileToBody(std::ifstream& file);
  void        _processGetRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processHeadRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processPostRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processPutRequest(HttpRequest& request, LocationInfo& location_info);
  void        _processDeleteRequest(HttpRequest& request, LocationInfo& location_info);
  void        _makeAutoIndexResponse(HttpRequest& request, LocationInfo& location_info, FileManager& file_manager);
  void        _makeErrorResponse(int error_code, Request& request, LocationInfo& location_info);
  void        _makeRedirResponse(int redir_code, HttpRequest& request, LocationInfo& location_info,
                                 const std::string& location_field = "");
  bool        _isAllowedMethod(int method, std::vector<std::string>& allowed_method);
  std::string _getMessage(int status_code);
  std::string _getContentType(const std::string& file_name);
  void        _makeResponse(vector::pointer body);

  // Constructor
 public:
  HttpResponse(HttpRequest& request, LocationInfo& location_info);
  HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info);
  // Interface
 public:
  bool               isSendingEnd() { return _sendingState == HTTP_SENDING_DONE; }
  void               sendResponse(int fd);
  int                contentLength() { return _contentLength; }
  const std::string& header() { return _header; }
  vector::pointer    body() { return _body; }
};

#endif
