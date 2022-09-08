#include "CgiResponse.hpp"

#include "HttpRequest.hpp"
#include "Log.hpp"
#include "syscall.hpp"

CgiResponse::CgiResponse()
    : _parsingState(CGI_RUNNING),
      _storage(),
      _httpVersion(),
      _statusCode(0),
      _statusMessage(),
      _contentType(),
      _body(NULL),
      _bodyFd(-1) {}

void CgiResponse::initialize() {
  _parsingState  = CGI_RUNNING;
  _statusCode    = 0;
  _httpVersion   = "";
  _statusMessage = "";
  _contentType   = "";
  _storage.clear();
  _body   = NULL;
  _bodyFd = -1;
  _getLine.initialize();
}

void CgiResponse::setInfo(const HttpRequest& http_requset) {
  _httpVersion         = http_requset.httpVersion();
  _method              = http_requset.method();
  _secretHeaderForTest = http_requset.secretHeaderForTest();
}

bool CgiResponse::_isCgiExecutionEnd(int pid, clock_t base_clock) {
  int status;
  int result = waitpid(pid, &status, WNOHANG);
  Log::log()(true, "pid", pid, INFILE);
  Log::log()(true, "result", result, INFILE);

  if (result == pid) {
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

void CgiResponse::readCgiResult(int fd, int pid, clock_t base_clock) {
  std::string line;
  switch (_parsingState) {
    case CGI_RUNNING:
      if (!_isCgiExecutionEnd(pid, base_clock))
        break;

    case CGI_READING_HEADER:
      line = _getLine.nextLine();
      while (_parseLine(line)) {
        line = _getLine.nextLine();
      }
      Log::log()(true, "statMsg", _statusMessage);
      Log::log()(true, "statcode", _statusCode);
      Log::log()(true, "_contentType", _contentType);
      Log::log()(true, "line.size", line.size());

      if (line.empty() || _statusMessage.empty() || _statusCode == 0 || _contentType.empty())
        break;

    default:
      int ret = ft::syscall::lseek(fd, 0, SEEK_CUR);
      Log::log()(true, "lseek", ret);
      int curr = ft::syscall::lseek(fd, static_cast<off_t>(_getLine.remainsCount() * -1), SEEK_CUR);
      ret      = ft::syscall::lseek(fd, 0, SEEK_CUR);
      Log::log()(true, "lseek", ret);

      int file_size = ft::syscall::lseek(fd, 0, SEEK_END);
      ret           = ft::syscall::lseek(fd, 0, SEEK_CUR);
      Log::log()(true, "lseek", ret);

      ft::syscall::lseek(fd, curr, SEEK_SET);
      ret = ft::syscall::lseek(fd, 0, SEEK_CUR);
      Log::log()(true, "lseek", ret);
      _bodySize     = file_size - curr;
      _bodyFd       = fd;
      _parsingState = CGI_PARSING_DONE;
      break;
  }
  Log::log()(LOG_LOCATION, "(STATE) CURRENT CGI_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
  Log::log()(true, "_bodySize", _bodySize, INFILE);
  Log::log()(true, "_bodyFd", _bodyFd, INFILE);
  Log::log()(true, "_getline.getFd", _getLine.getFd(), INFILE);
  Log::log()(true, "_getline.remainsCount", _getLine.remainsCount(), INFILE);
}

// void CgiResponse::_parseCgiResponse(clock_t base_clock) {
//   std::string       line = _storage.getLine();
//   std::stringstream ss;

//   while (line != "") {
//     if (line.find("Status:") != std::string::npos) {
//       ss << line.substr(8);
//       ss >> _statusCode;
//       ss >> _statusMessage;
//     } else if (line.find("Content-Type:") != std::string::npos) {
//       ss << line.substr(14);
//       std::string content_type_line;
//       ss >> content_type_line;
//       _contentType += content_type_line;
//       _contentType += " ";
//       ss >> content_type_line;
//       _contentType += content_type_line;
//     } else if (line.find("\r") != std::string::npos) {
//       _body = _storage.currentReadPos();
//       break;
//     }
//     line = _storage.getLine();
//   }
//   _parsingState = CGI_PARSING_DONE;
//   Log::log()(true, "CGI RESULT PARSING     DONE TIME", (double)(clock() - base_clock) / CLOCKS_PER_SEC, ALL);
// }

// std::string CgiResponse::CgiResponseResultString() {
//   std::stringstream ss;
//   ss << "Status-Code: [" << _statusCode << "]" << std::endl;
//   ss << "Status-Message: [" << _statusMessage << "]" << std::endl;
//   ss << "Content-Type: [" << _contentType << "]" << std::endl;
//   // ss << "----Body---" << std::endl << _body << std::endl << "-----------" << std::endl;
//   return ss.str();
// }

bool CgiResponse::isExecuteError() { return _parsingState == CGI_ERROR; }
bool CgiResponse::isReadError() { return _getLine.isReadError(); }

bool CgiResponse::isParsingEnd() { return _parsingState == CGI_ERROR || _parsingState == CGI_PARSING_DONE; }

// std::vector<std::string> CgiResponse::_split(const std::string& str, const std::string& delimiter) {
//   std::vector<std::string> _result;
//   std::string::size_type   _prev_pos = 0;
//   std::string::size_type   _pos      = 0;

//   while ((_pos = str.find(delimiter, _prev_pos)) != std::string::npos) {
//     _result.push_back(str.substr(_prev_pos, _pos - _prev_pos));
//     _prev_pos = _pos + delimiter.size();
//   }
//   _result.push_back(str.substr(_prev_pos));
//   return _result;
// }
