#ifndef HTTPRESPONSE_HPP

#include <fstream>
#include <string>
#include <vector>

#define DEFAULT_ERROR_PAGE_DIR "error_pages"

class Request;
class HttpRequest;
class CgiResponse;
class FileManager;
struct LocationInfo;

enum HttpResponseSendingState {
  HTTP_SENDING_HEADER,
  HTTP_SENDING_TEMPBODY,
  HTTP_SENDING_FILEBODY,
  HTTP_SENDING_DONE,
  HTTP_SENDING_CONNECTION_CLOSED
};

class HttpResponse {
  // Types
  typedef std::vector<unsigned char> vector;

  // Member Variable
 private:
  // vector::pointer               _body;
  vector                        _tempBody;
  enum HttpResponseSendingState _sendingState;
  int                           _bodyFd;
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
  bool        _sendingStateTransition(size_t total_size, size_t already_sent, HttpResponseSendingState state);
  template <typename T>
  void _sendChunk(int fd, const T* base, size_t total_size, size_t& already_sent);

  // Constructor
 public:
  HttpResponse(HttpRequest& request, LocationInfo& location_info);
  HttpResponse(CgiResponse& cgi_response, LocationInfo& location_info);
  // Interface
 public:
  bool               isConnectionClosed() { return _sendingState == HTTP_SENDING_CONNECTION_CLOSED; }
  bool               isSendingEnd() { return _sendingState == HTTP_SENDING_DONE; }
  void               sendResponse(int fd);
  int                contentLength() { return _contentLength; }
  const std::string& header() { return _header; }
  int                bodyFd() { return _bodyFd; }
  // vector::pointer    body() { return _body; }
};

#endif
