#ifndef CGIRESPONSE_HPP

class CgiResponse {
  // Constructor
 public:
  CgiResponse() {}

  // Interface
 public:
  bool isEnd() { return true; }
  void add(int client_fd) { (void)client_fd; }
};

#endif