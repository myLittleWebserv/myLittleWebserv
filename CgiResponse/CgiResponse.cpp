#include "CgiResponse.hpp"

void CgiResponse::_parseCgiResponse(std::string& cgi_result) {
  std::stringstream buffer(cgi_result);
  std::string       line;

  while (std::getline(buffer, line)) {
    if (line.find("Status:") != std::string::npos) {
      std::string status_line = line.substr(8);
      status_line[status_line.size() - 1] = '\0';
      std::vector<std::string> status_line_split = _split(status_line, " ");
      _statusCode = atoi(status_line_split[0].c_str());
      _statusMessage = status_line_split[1];
    }
    else if (line.find("Content-Type:") != std::string::npos) {
      std::string content_type_line = line.substr(14);
      content_type_line[content_type_line.size() - 1] = '\0';
      _contentType = content_type_line;
    }
    else if (line.find("\r") != std::string::npos) {
      continue;
    }
    else {
      _body += (line + "\n");
    }
  }
  _printCgiResponse();
}

void CgiResponse::_printCgiResponse() {
  std::cout << "Status-Code: [" << _statusCode << "]" << std::endl;
  std::cout << "Status-Message: [" << _statusMessage << "]" << std::endl;
  std::cout << "Content-Type: [" << _contentType << "]" << std::endl;
  std::cout << "----Body---" << std::endl <<_body;
}

bool CgiResponse::isEnd() { return _isEnd; }

void CgiResponse::readCgiResult(int pipe_fd) {
  // open a filestream from pipe_fd
  // read the filestream into cgi_result
  // parse the cgi_result
}

void CgiResponse::setCgiEnd() { _isEnd = true; }

int CgiResponse::getStatusCode() { return _statusCode; }

std::string CgiResponse::getStatusMessage() { return _statusMessage; }

std::string CgiResponse::getContentType() { return _contentType; }

std::string CgiResponse::getBody() { return _body; }

CgiResponse::CgiResponse() {
  _isEnd         = false;
  _statusCode    = 0;
  _statusMessage = "";
  _contentType   = "";
  _body          = "";
}

std::vector<std::string> CgiResponse::_split(const std::string& str, const std::string& delimiter) {
  std::vector<std::string> _result;
  std::string::size_type   _prev_pos = 0;
  std::string::size_type   _pos      = 0;

  while ((_pos = str.find(delimiter, _prev_pos)) != std::string::npos) {
    _result.push_back(str.substr(_prev_pos, _pos - _prev_pos));
    _prev_pos = _pos + delimiter.size();
  }
  _result.push_back(str.substr(_prev_pos));
  return _result;
}
