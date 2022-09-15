#include "CgiResponse.hpp"

#include "Event.hpp"
#include "HttpRequest.hpp"
#include "Log.hpp"
#include "syscall.hpp"

CgiResponse::CgiResponse()
    : _parsingState(CGI_RUNNING),
      _pid(-1),
      _httpVersion(),
      _statusCode(0),
      _statusMessage(),
      _contentType(),
      _bodyFd(-1),
      _bodySize(0) {}

void CgiResponse::initialize() {
  _parsingState  = CGI_RUNNING;
  _pid           = -1;
  _statusCode    = 0;
  _httpVersion   = "";
  _statusMessage = "";
  _contentType   = "";
  _bodyFd        = -1;
  _bodySize      = 0;
  _storage.clear();
}

void CgiResponse::setInfo(const HttpRequest& http_requset) {
  _httpVersion         = http_requset.httpVersion();
  _method              = http_requset.method();
  _secretHeaderForTest = http_requset.secretHeaderForTest();
}

bool CgiResponse::_isCgiExecutionEnd(clock_t base_clock) {
  int status;
  int result = waitpid(_pid, &status, WNOHANG);
  // Log::log()(true, "pid", _pid, INFILE);
  // Log::log()(true, "result", result, INFILE);

  if (result == _pid) {
    _parsingState = CGI_PARSING_HEADER;
    Log::log()(true, "CGI EXECUTION          DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, CONSOLE);
    return true;
  } else if (result == -1) {
    _parsingState = CGI_ERROR;
    Log::log()(LOG_LOCATION, "CGI_ERROR", INFILE);
  }
  return false;
}

bool CgiResponse::_parseLine(const std::string& line) {
  // Log::log()(LOG_LOCATION, "");
  // Log::log()(true, "line", line);
  if (line.empty() || line == "\r") {
    return false;
  }
  std::stringstream ss(line);
  std::string       word;
  ss >> word;
  if (word == "Status:") {
    ss >> _statusCode;
    ss >> _statusMessage;
  } else if (word == "Content-Type:") {
    std::getline(ss, _contentType, '\r');
  }
  return true;
}

void CgiResponse::parseRequest(int recv_fd, clock_t base_clock) {
  int         curr;
  int         file_size;
  std::string line;

  // Log::log()(true, "recv_fd", recv_fd);
  switch (_parsingState) {
    case CGI_RUNNING:
      if (!_isCgiExecutionEnd(base_clock))
        break;

    case CGI_PARSING_HEADER:
      line = _storage.getLineFile(recv_fd);
      while (_parseLine(line)) {
        line = _storage.getLineFile(recv_fd);
      }

      if (line.empty() || _statusMessage.empty() || _statusCode == 0 || _contentType.empty())
        break;

    case CGI_PARSING_DONE:
      curr      = ft::syscall::lseek(recv_fd, static_cast<off_t>(_storage.remains()) * -1, SEEK_CUR);
      file_size = ft::syscall::lseek(recv_fd, 0, SEEK_END);
      ft::syscall::lseek(recv_fd, curr, SEEK_SET);
      _bodySize     = file_size - curr;
      _parsingState = CGI_PARSING_DONE;
      break;

    default:
      break;
  }
  Log::log()(LOG_LOCATION, "(STATE) CURRENT CGI_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
}
