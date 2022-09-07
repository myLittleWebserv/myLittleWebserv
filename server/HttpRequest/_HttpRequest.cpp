#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "HttpRequest.hpp"
#include "Log.hpp"

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
  _body.clear();
  _storage.preserveRemains();
}

bool HttpRequest::isParsingEnd() {
  return _parsingState == HTTP_PARSING_DONE || _parsingState == BAD_REQUEST || _parsingState == TIME_OUT;
}

bool HttpRequest::isCgi(const std::string& ext) {
  std::string::size_type ext_delim = _uri.rfind('.');
  if (std::string::npos == ext_delim || _uri.substr(ext_delim) != ext) {
    return false;
  }
  return true;
}

void HttpRequest::storeChunk(int fd) {
  _storage.readFile(fd);
  if (_storage.state() != RECEIVE_DONE) {
    return;
  }

  Log::log()(LOG_LOCATION, "(TRANSFER) socket buffer to _storage done", INFILE);

  std::string line;

  switch (_parsingState) {
    case HTTP_PARSING_INIT:
      line = _storage.getLine();  // 라인이 완성되어 있지 않으면 "" 리턴
      _parseStartLine(line);
      if (line.empty() || isParsingEnd())
        break;

    case HTTP_PARSING_HEADER:
      while (_parsingState == HTTP_PARSING_HEADER && !(line = _storage.getLine()).empty()) {  // ㅎㅡㅁ ..
        _parseHeaderField(line);
      }
      if (line.empty() || isParsingEnd())
        break;

    case HTTP_PARSING_BODY:
      if (_isChunked) {
        _parseChunk();
      } else if (_storage.remains() > _contentLength) {
        _storage.dataToBody(_body, _contentLength);
        _parsingState = HTTP_PARSING_DONE;
      }

    default:
      _checkTimeOut();
      break;
  }

  Log::log()(LOG_LOCATION, "(STATE) CURRENT HTTP_PARSING STATE", INFILE);
  Log::log()("_parsingState", _parsingState, INFILE);
  Log::log()(true, "_body.size", _body.size(), INFILE);
  Log::log()(true, "_storage.size", _storage.size(), INFILE);
}

// Method

void HttpRequest::_checkTimeOut() {
  if (time(NULL) - _timeStamp >= HTTP_PARSING_TIME_OUT) {
    Log::log()(LOG_LOCATION, "TIME OUT", INFILE);
    _parsingState = TIME_OUT;
  }
}

void HttpRequest::_parseStartLine(const std::string& line) {
  if (line.empty()) {
    return;
  }
  if (*line.rbegin() != '\r' || std::count(line.begin(), line.end(), ' ') != 2) {
    Log::log()(LOG_LOCATION, "", INFILE);
    Log::log()(true, "BAD_REQUEST: line", line, INFILE);
    Log::log()(true, "BAD_REQUEST: line.size", line.size(), INFILE);
    Log::log()(true, "BAD_REQUEST: line[0]", (int)(line.c_str()[0]), INFILE);
    _parsingState = BAD_REQUEST;
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
    Log::log()(LOG_LOCATION, "", INFILE);
    Log::log()(true, "BAD_REQUEST: line", line, INFILE);
    _parsingState = BAD_REQUEST;
  } else {
    _parsingState = HTTP_PARSING_HEADER;
  }
}

void HttpRequest::_parseHeaderField(const std::string& line) {
  if (line.empty()) {
    return;
  }
  if (line == "\r") {
    _parsingState = _isBodyExisted ? HTTP_PARSING_BODY : HTTP_PARSING_DONE;
    return;
  }
  if (*line.rbegin() != '\r' || line.size() + 1 + _headerSize > HTTP_MAX_HEADER_SIZE) {
    _parsingState = BAD_REQUEST;
    return;
  }

  _headerSize += line.size() + 1;
  _timeStamp = time(NULL);

  std::stringstream ss(line);
  std::string       word;

  // std::getline(ss, word, ' ');
  ss >> word;
  if (word == "Content-Type:") {
    ss >> _contentType;
  } else if (word == "Content-Length:") {
    ss >> _contentLength;
    _isBodyExisted = true;
  } else if (word == "Host:") {
    ss >> word;
    std::string::size_type delim = word.find(':');
    if (delim == std::string::npos) {
      _hostPort = HTTP_DEFAULT_PORT;
      _hostName = word;
    } else {
      _hostPort = std::atoi(word.substr(delim + 1).c_str());
      _hostName = word.substr(0, delim);
    }
  } else if (word == "Connection:") {
    Log::log()(true, "header-field", word);
    ss >> word;
    if (word == "keep-alive") {
      _isKeepAlive = true;
    } else {
      _isKeepAlive = false;
    }
    Log::log()(true, "header-value", word);
  } else if (word == "Transfer-Encoding:") {
    ss >> word;
    if (word == "chunked") {
      _isChunked     = true;
      _isBodyExisted = true;
    }
  } else if (word == "X-Secret-Header-For-Test:") {
    ss >> _secretHeaderForTest;
  } else {
    Log::log()(LOG_LOCATION, "");
    Log::log()(true, "header-field", word);
    ss >> word;
    Log::log()(true, "header-value", word);
  }
}

void HttpRequest::_parseChunk() {
  while (1) {
    if (_chunkSize == -1) {
      std::string line = _storage.getLine();
      if (line.empty()) {
        return;
      }
      _chunkSize = _parseChunkSize(line);
    }

    if (_chunkSize == 0) {
      _parsingState = HTTP_PARSING_DONE;
      _storage.moveReadPos(2);
      _chunkSize = -1;
      return;
    }

    if (_storage.remains() > _chunkSize) {
      _storage.dataToBody(_body, _chunkSize, _isChunked);
      _chunkSize = -1;
    } else {
      break;
    }
  }
  _timeStamp = time(NULL);
}

size_t HttpRequest::_parseChunkSize(const std::string& line) {
  char*  p;
  size_t size = std::strtol(line.c_str(), &p, 16);  // chunk 최대 크기 몇?
  if (*p != '\r') {
    _parsingState = BAD_REQUEST;
    Log::log()(LOG_LOCATION, "", INFILE);
    Log::log()(true, "BAD_REQUEST: line", line, INFILE);
    return 0;
  }
  return size;
}
