#include "CgiResponse.hpp"

    void CgiResponse::_parseCgiResponse(std::string& cgi_result) {

    }

  CgiResponse::CgiResponse() {
    _isEnd = false;
    _statusCode = 0;
    _statusMessage = "";
    _contentType = "";
    _body = "";
  }

  bool CgiResponse::isEnd() {

  }
  void CgiResponse::readCgiResult(int pipe_fd) {

  }

  void CgiResponse::setCgiEnd() {

  }
