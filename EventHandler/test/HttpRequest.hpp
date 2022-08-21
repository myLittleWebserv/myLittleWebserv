#ifndef HTTPREQUEST_HPP

#include <sys/socket.h>

#include <sstream>

struct HttpRequest {
  int _hostPort;

  // Constructor
 public:
  HttpRequest() {}

  // Interface
 public:
  bool isEnd() { return true; }
  void add(int client_fd) {
    unsigned char buffer[8000];
    int           recv_size = recv(client_fd, buffer, 8000, 0);

    buffer[recv_size] = 0;
    std::stringstream ss;
    ss << buffer;
    std::string line;
    std::getline(ss, line);
    if (line == "server1") {
      _hostPort = 7777;
    } else {
      _hostPort = 1000;
    }
  }
};

#endif