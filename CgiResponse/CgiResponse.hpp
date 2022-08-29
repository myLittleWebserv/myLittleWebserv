#ifndef CGIRESPONSE_HPP

#include <string>
#include <sstream>

class CgiResponse {

  private:
    bool _isEnd;
    int _statusCode;
    std::string _statusMessage;
    std::string _contentType;
    std::string _body;

    void _parseCgiResponse(std::string& cgi_results);

  // Constructor
 public:
  CgiResponse() {}

  // Interface
 public:
  bool isEnd();
  void readCgiResult(int pipe_fd);
  void setCgiEnd();
};

#endif
