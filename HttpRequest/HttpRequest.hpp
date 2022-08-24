#ifndef HTTPREQUEST_HPP

#include <sys/socket.h>

#include <sstream>

#include "Log.hpp"

#define BUFFER_SIZE 1000

enum HttpRequestParsingState { RECEIVING, CONNECTION_CLOSED, HEADER_END, BODY_END, PARSING_DONE, BAD_REQUEST };

class HttpRequest {
  int         _hostPort;
  std::string _hostName;
  bool        _keepAlive;
  bool        _end;
  bool        _badRequest;
  bool        _connectionClosed;

  // Constructor
 public:
  HttpRequest() : _hostPort(7777), _hostName("for test"), _keepAlive(true), _end(true), _connectionClosed(false) {}

  // Interface
 public:
  bool& isEnd() { return _end; }
  bool  isCgi(const std::string& cgi_extension) {
     (void)cgi_extension;
     return false;
  }
  bool               isConnectionClosed() { return _connectionClosed; }
  bool               isKeepAlive() { return _keepAlive; }
  int                hostPort() { return _hostPort; }
  const std::string& hostName() { return _hostName; }

  void storeChunk(int client_fd) {
    unsigned char buf[BUFFER_SIZE];
    int           recv_size = recv(client_fd, buf, BUFFER_SIZE, 0);
    Log::log()("recv_size", recv_size, ALL);
    if (recv_size == 0) {
      _connectionClosed = true;
      return;
    }
  }
};

#endif
