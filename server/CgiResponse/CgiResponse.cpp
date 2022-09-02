#include "CgiResponse.hpp"

CgiResponse::CgiResponse() {
  _storage       = CgiStorage();
  _isParsingEnd  = false;
  _isError       = false;
  _statusCode    = 0;
  _statusMessage = "";
  _contentType   = "";
}

void CgiResponse::readCgiResult(int fd, int pid) {
  int status         = 0;
  int waitpid_result = waitpid(pid, &status, WNOHANG);
  if (waitpid_result != pid) {
    return;
  }
  if (WIFEXITED(status)) {
    if (WEXITSTATUS(status) != 0) {
      _isError = true;
      return;
    }
  } else {
    _isError = true;
    return;
  }
  _storage.readFd(fd);

  // for test
  //  while (_storage.isReadingEnd() == false) {
  //    _storage.readFd(fd);
  //  }
  if (_storage.isReadingEnd() && !_isParsingEnd) {
    _parseCgiResponse();
  }
}

void CgiResponse::_parseCgiResponse() {
  std::string line = _storage.getLine();

  while (line != "") {
    if (line.find("Status:") != std::string::npos) {
      std::string status_line                    = line.substr(8);
      status_line[status_line.size() - 1]        = '\0';
      std::vector<std::string> status_line_split = _split(status_line, " ");
      _statusCode                                = atoi(status_line_split[0].c_str());
      _statusMessage                             = status_line_split[1];
    } else if (line.find("Content-Type:") != std::string::npos) {
      std::string content_type_line                   = line.substr(14);
      content_type_line[content_type_line.size() - 1] = '\0';
      _contentType                                    = content_type_line;
    } else if (line.find("\r") != std::string::npos) {
      _body = _storage.remainder();
      break;
    }
    line = _storage.getLine();
  }
  _isParsingEnd = true;
}

std::string CgiResponse::CgiResponseResultString() {
  std::stringstream ss;
  ss << "Status-Code: [" << _statusCode << "]" << std::endl;
  ss << "Status-Message: [" << _statusMessage << "]" << std::endl;
  ss << "Content-Type: [" << _contentType << "]" << std::endl;
  ss << "----Body---" << std::endl << _body << std::endl << "-----------" << std::endl;
  return ss.str();
}

int CgiResponse::getStatusCode() { return _statusCode; }

std::string CgiResponse::getStatusMessage() { return _statusMessage; }

std::string CgiResponse::getContentType() { return _contentType; }

const std::vector<unsigned char>& CgiResponse::getBody() { return _body; }

bool CgiResponse::isError() { return _isError; }

bool CgiResponse::isParsingEnd() { return _isParsingEnd; }

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
