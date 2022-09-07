#include "CgiResponse.hpp"

#include "HttpRequest.hpp"
#include "Log.hpp"

CgiResponse::CgiResponse()
    : _parsingState(CGI_RUNNING),
      _storage(),
      _httpVersion(),
      _statusCode(0),
      _statusMessage(),
      _contentType(),
      _body(NULL) {}

void CgiResponse::initialize() {
  _parsingState = CGI_RUNNING;
  _statusCode   = 0;

  _storage.clear();
  _body = NULL;
}

void CgiResponse::setInfo(const HttpRequest& http_requset) {
  _httpVersion         = http_requset.httpVersion();
  _method              = http_requset.method();
  _secretHeaderForTest = http_requset.secretHeaderForTest();
}

void CgiResponse::_checkWaitPid(int pid, clock_t base_clock) {
  int status;
  int result = waitpid(pid, &status, WNOHANG);
  Log::log()(true, "pid", pid, INFILE);
  Log::log()(true, "result", result, INFILE);

  if (result == pid) {
    _parsingState = CGI_READING;
    Log::log()(true, "CGI EXECUTION          DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, ALL);
  } else if (result == -1) {
    _parsingState = CGI_ERROR;
    Log::log()(LOG_LOCATION, "CGI_ERROR", INFILE);
  }
}

void CgiResponse::readCgiResult(int fd, int pid, clock_t base_clock) {
  if (_parsingState == CGI_RUNNING) {
    _checkWaitPid(pid, base_clock);
  }

  if (_parsingState == CGI_READING) {
    _storage.readFile(fd);
    if (_storage.isReadingEnd()) {
      _parsingState = CGI_PARSING;
      Log::log()(LOG_LOCATION, "(DONE) CGI RESULT READING", INFILE);
      Log::log()(true, "CGI RESULT READING     DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, ALL);
    }
  }

  if (_parsingState == CGI_PARSING) {
    _parseCgiResponse(base_clock);
  }
  Log::log()(LOG_LOCATION, "(STATE) CURRENT CGI_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
  Log::log()(true, "_storage.size", _storage.size(), INFILE);
}

void CgiResponse::_parseCgiResponse(clock_t base_clock) {
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
      _body = _storage.currentReadPos();
      break;
    }
    line = _storage.getLine();
  }
  _parsingState = CGI_PARSING_DONE;
  Log::log()(true, "CGI RESULT PARSING     DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, ALL);
}

std::string CgiResponse::CgiResponseResultString() {
  std::stringstream ss;
  ss << "Status-Code: [" << _statusCode << "]" << std::endl;
  ss << "Status-Message: [" << _statusMessage << "]" << std::endl;
  ss << "Content-Type: [" << _contentType << "]" << std::endl;
  // ss << "----Body---" << std::endl << _body << std::endl << "-----------" << std::endl;
  return ss.str();
}

bool CgiResponse::isError() { return _parsingState == CGI_ERROR; }

bool CgiResponse::isParsingEnd() { return _parsingState == CGI_ERROR || _parsingState == CGI_PARSING_DONE; }

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
