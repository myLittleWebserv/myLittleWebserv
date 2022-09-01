#include "CgiResponse.hpp"

CgiResponse::CgiResponse() {
  _storage       = CgiStorage();
  _isParsingEnd  = false;
  _statusCode    = 0;
  _statusMessage = "";
  _contentType   = "";
  _body          = "";
}

void CgiResponse::readCgiResult(int fd, int pid) {
  // 프로세스 exit status 체크해서 Cgi Status를 결정해줘야할듯?
  // -> TODO: 객체 내부에 에러를 체크할 수 있는 인터페이스를 만들어 줄 것.  
  int waitpid_result = waitpid(pid, NULL, WNOHANG);
  if (waitpid_result != pid) {
    return;
  }
  _storage.readFd(fd);

  //for test
  // while (_storage.isReadingEnd() == false) {
  //   _storage.readFd(fd);
  // }
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

// TODO: body unsinged char 벡터로 바꾸고, 레퍼런스로 바꾸기/
std::string CgiResponse::getBody() { return _body; }

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
