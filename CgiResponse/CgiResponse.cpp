#include "CgiResponse.hpp"

    void CgiResponse::_parseCgiResponse(std::string& cgi_result) {

    }

  bool CgiResponse::isEnd() {
    return _isEnd;
  }
  void CgiResponse::readCgiResult(int pipe_fd) {
    std::string cgi_result;
    int red;
    char buf[1024];
    red = read(pipe_fd, buf, 1024);
    cgi_result = buf;
    _parseCgiResponse(cgi_result);
  }

  void CgiResponse::setCgiEnd() {
    _isEnd = true;
  }

  CgiResponse::CgiResponse() {
    _isEnd = false;
    _statusCode = 0;
    _statusMessage = "";
    _contentType = "";
    _body = "";
  }