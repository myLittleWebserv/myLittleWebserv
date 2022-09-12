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
  _getLine.initialize();
}

void CgiResponse::setInfo(const HttpRequest& http_requset) {
  _httpVersion         = http_requset.httpVersion();
  _method              = http_requset.method();
  _secretHeaderForTest = http_requset.secretHeaderForTest();
}

bool CgiResponse::_isCgiExecutionEnd(clock_t base_clock) {
  int status;
  int result = waitpid(_pid, &status, WNOHANG);
  Log::log()(true, "pid", _pid, INFILE);
  Log::log()(true, "result", result, INFILE);

  if (result == _pid) {
    _parsingState = CGI_READING_HEADER;
    Log::log()(true, "CGI EXECUTION          DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, ALL);
    return true;
  } else if (result == -1) {
    _parsingState = CGI_ERROR;
    Log::log()(LOG_LOCATION, "CGI_ERROR", INFILE);
  }
  return false;
}

bool CgiResponse::_parseLine(const std::string& line) {
  Log::log()(LOG_LOCATION, "");
  Log::log()(true, "line", line);
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
  switch (_parsingState) {
    case CGI_RUNNING:
      if (!_isCgiExecutionEnd(base_clock))
        break;

    case CGI_READING_HEADER:
      line = _getLine.nextLine();
      while (_parseLine(line)) {
        line = _getLine.nextLine();
      }
      Log::log()(true, "line.size", line.size());

      if (line.empty() || _statusMessage.empty() || _statusCode == 0 || _contentType.empty())
        break;

    case CGI_PARSING_DONE:
      curr      = ft::syscall::lseek(recv_fd, static_cast<off_t>(_getLine.remainsCount() * -1), SEEK_CUR);
      file_size = ft::syscall::lseek(recv_fd, 0, SEEK_END);
      ft::syscall::lseek(recv_fd, curr, SEEK_SET);
      Log::log()(true, "curr offset", curr);
      Log::log()(true, "file_size", file_size);
      _bodySize     = file_size - curr;
      _bodyFd       = recv_fd;
      _parsingState = CGI_PARSING_DONE;
      break;

    default:
      break;
  }
  Log::log()(LOG_LOCATION, "(STATE) CURRENT CGI_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
  Log::log()(true, "_bodySize", _bodySize, INFILE);
  Log::log()(true, "_bodyFd", _bodyFd, INFILE);
}
