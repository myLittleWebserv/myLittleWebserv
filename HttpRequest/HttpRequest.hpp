#ifndef HTTPREQUEST_HPP

#include <sys/socket.h>

#include <sstream>

class HttpRequest {
  int         _hostPort;
  std::string _hostName;
  bool        _keepAlive;

  // Constructor
 public:
  HttpRequest() {}

  // Interface
 public:
  bool isEnd() { return true; }
  bool isCgi(const std::string& cgi_extension) {
    (void)cgi_extension;
    return false;
  }
  bool               isKeepAlive() { return _keepAlive; }
  int                hostPort() { return _hostPort; }
  const std::string& hostName() { return _hostName; }
  void               storeChunk(int client_fd) { (void)client_fd; }
};

#endif
