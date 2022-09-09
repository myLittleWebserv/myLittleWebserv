#include "HttpRequest.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "Event.hpp"
#include "GetLine.hpp"
#include "Log.hpp"
#include "syscall.hpp"
// Interface

void HttpRequest::initialize() {
  _method              = NOT_IMPL;
  _uri                 = "";
  _contentLength       = 0;
  _contentType         = "";
  _hostName            = "";
  _hostPort            = HTTP_DEFAULT_PORT;
  _parsingState        = HTTP_PARSING_INIT;
  _headerSize          = 0;
  _timeStamp           = time(NULL);
  _isBodyExisted       = false;
  _isChunked           = false;
  _isKeepAlive         = true;
  _chunkSize           = -1;
  _secretHeaderForTest = 0;
  _bodyFd              = -1;
  _bodySize            = 0;
  _getLine.clear();
}

bool HttpRequest::isParsingEnd() {
  return _parsingState == HTTP_PARSING_DONE || _parsingState == HTTP_PARSING_BAD_REQUEST ||
         _parsingState == HTTP_PARSING_TIME_OUT;
}

bool HttpRequest::isCgi(const std::string& ext) {
  std::string::size_type ext_delim = _uri.rfind('.');
  if (std::string::npos == ext_delim || _uri.substr(ext_delim) != ext) {
    return false;
  }
  return true;
}

void HttpRequest::parseRequest(Event& event) {
  int         recv_fd = event.toRecvFd;
  std::string line;
  int         curr;
  int         file_size;

  switch (_parsingState) {
    case HTTP_PARSING_INIT:
      line = _getLine.nextLine();  // 라인이 완성되어 있지 않으면 "" 리턴
      _parseStartLine(line);
      if (line.empty() || isParsingEnd())
        break;

    case HTTP_PARSING_HEADER:
      line = _getLine.nextLine();
      while (_parseHeaderField(line)) {
        line = _getLine.nextLine();
      }
      if (line.empty() || isParsingEnd())
        break;

    case HTTP_PARSING_BODY:
      _bodyFd = recv_fd;
      if (!_isChunked) {
        curr      = ft::syscall::lseek(recv_fd, static_cast<off_t>(_getLine.remainsCount() * -1), SEEK_CUR);
        file_size = ft::syscall::lseek(recv_fd, 0, SEEK_END);
        ft::syscall::lseek(recv_fd, curr, SEEK_SET);
        _bodySize     = file_size - curr;
        _parsingState = HTTP_PARSING_DONE;
        break;
      }

    case HTTP_PARSING_BODY_CHUNK:
      line = _getLine.nextLine();
      if (line.empty()) {
        _parsingState = HTTP_PARSING_BODY_CHUNK;
        break;
      }
      ft::syscall::lseek(recv_fd, static_cast<off_t>(_getLine.remainsCount() * -1), SEEK_CUR);
      _bodySize = std::atoi(line.c_str());

    default:
      _parsingState = HTTP_PARSING_DONE;
      _checkTimeOut();
      break;
  }

  Log::log()(LOG_LOCATION, "(STATE) CURRENT HTTP_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
}

// Method

void HttpRequest::_checkTimeOut() {
  if (time(NULL) - _timeStamp >= TIME_OUT_HTTP_PARSING) {
    Log::log()(LOG_LOCATION, "TIME OUT", INFILE);
    _parsingState = HTTP_PARSING_TIME_OUT;
  }
}

void HttpRequest::_parseStartLine(const std::string& line) {
  if (line.empty()) {
    return;
  }
  if (*line.rbegin() != '\r' || std::count(line.begin(), line.end(), ' ') != 2) {
    Log::log()(LOG_LOCATION, "BAD_REQUEST", INFILE);
    Log::log()(true, "line", line, INFILE);
    _parsingState = HTTP_PARSING_BAD_REQUEST;
    return;
  }

  _headerSize += line.size() + 1;
  _timeStamp = time(NULL);

  std::stringstream ss(line);
  std::string       word;

  std::getline(ss, word, ' ');
  if (word == "GET") {
    _method = GET;
  } else if (word == "HEAD") {
    _method = HEAD;
  } else if (word == "POST") {
    _method = POST;
  } else if (word == "PUT") {
    _method = PUT;
  } else if (word == "DELETE") {
    _method = DELETE;
  } else {
    _method = NOT_IMPL;
  }

  std::getline(ss, _uri, ' ');
  std::getline(ss, _httpVersion, '\r');

  if (ss.bad()) {
    Log::log()(LOG_LOCATION, "BAD_REQUEST", INFILE);
    Log::log()(true, "line", line, INFILE);
    _parsingState = HTTP_PARSING_BAD_REQUEST;
  } else {
    _parsingState = HTTP_PARSING_HEADER;
  }
}

bool HttpRequest::_parseHeaderField(const std::string& line) {
  if (line.empty()) {
    return false;
  }
  if (line == "\r") {
    _parsingState = _isBodyExisted ? HTTP_PARSING_BODY : HTTP_PARSING_DONE;
    return false;
  }
  if (*line.rbegin() != '\r' || line.size() + 1 + _headerSize > HTTP_MAX_HEADER_SIZE) {
    _parsingState = HTTP_PARSING_BAD_REQUEST;
    return false;
  }

  _headerSize += line.size() + 1;
  _timeStamp = time(NULL);

  std::stringstream ss(line);
  std::string       word;

  std::getline(ss, word, ' ');
  if (word == "Content-Type:") {
    std::getline(ss, _contentType, '\r');
  } else if (word == "Content-Length:") {
    ss >> _contentLength;
    _isBodyExisted = true;
  } else if (word == "Host:") {
    std::getline(ss, word, '\r');
    std::string::size_type delim = word.find(':');
    if (delim == std::string::npos) {
      _hostPort = HTTP_DEFAULT_PORT;
      _hostName = word;
    } else {
      _hostPort = std::atoi(word.substr(delim + 1).c_str());
      _hostName = word.substr(0, delim);
    }
  } else if (word == "Connection:") {
    std::getline(ss, word, '\r');
    if (word == "keep-alive") {
      _isKeepAlive = true;
    } else {
      _isKeepAlive = false;
    }
    Log::log()(true, "header-value", word);
  } else if (word == "Transfer-Encoding:") {
    std::getline(ss, word, '\r');
    if (word == "chunked") {
      _isChunked     = true;
      _isBodyExisted = true;
    }
  } else if (word == "X-Secret-Header-For-Test:") {
    ss >> _secretHeaderForTest;
  } else {
    Log::log()(true, "header-field", word);
    ss >> word;
    Log::log()(true, "header-value", word);
  }
  return true;
}

void HttpRequest::_parseChunk() {
  while (1) {
    if (_chunkSize == -1) {
      std::string line = _getLine.nextLine();
      if (line.empty()) {
        return;
      }
      _chunkSize = _parseChunkSize(line);
    }

    if (_chunkSize == 0) {
      std::string line = _getLine.nextLine();
      if (line.empty()) {
        return;
      }
      _parsingState = HTTP_PARSING_DONE;
      _chunkSize    = -1;
      return;
    }

    // if (_getLine.remains() > _chunkSize) {  // ?
    //   _storage.dataToBody(_body, _chunkSize, _isChunked);
    //   _chunkSize = -1;
    // } else {
    //   break;
    // }
  }
  _timeStamp = time(NULL);
}

size_t HttpRequest::_parseChunkSize(const std::string& line) {
  char*  p;
  size_t size = std::strtol(line.c_str(), &p, 16);  // chunk 최대 크기 몇?
  if (*p != '\r') {
    _parsingState = HTTP_PARSING_BAD_REQUEST;
    Log::log()(LOG_LOCATION, "BAD_REQUEST", INFILE);
    Log::log()(true, "line", line, INFILE);
    return 0;
  }
  return size;
}
