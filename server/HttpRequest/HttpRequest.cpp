#include "HttpRequest.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "Log.hpp"

// Interface

bool HttpRequest::isCgi(const std::string& ext) {
  std::string::size_type ext_delim = _uri.rfind('.');
  if (std::string::npos == ext_delim || _uri.substr(ext_delim + 1) != ext) {
    return false;
  }
  return true;
}

void HttpRequest::storeChunk(int fd) {
  _storage.readSocket(fd);
  if (_storage.state() != RECEIVE_DONE) {
    return;
  }

  Log::log()(LOG_LOCATION, "(TRANSFER) socket buffer to _storage done", ALL);

  if (_parsingState == PARSING_INIT || _parsingState == PARSING_HEADER) {
    _parseHeader();
  }
  if (_parsingState == PARSING_BODY) {
    _parseBody();
  }

  Log::log()(LOG_LOCATION, "(STATE) CURRENT PARSING STATE", ALL);
  Log::log()("_parsingState", _parsingState, ALL);
  Log::log()(true, "_body.size", _body.size(), ALL);
  Log::log()(true, "_storage.size", _storage.size(), ALL);
}

void HttpRequest::initialize() {
  _parsingState    = PARSING_INIT;
  _headerSize      = 0;
  _headerTimeStamp = clock();
  _bodyTimeStamp   = _headerTimeStamp;
  _isBodyExisted   = false;
  _isChunked       = false;
  _chunkSize       = -1;
  _body.clear();
  _storage.clear();
}

// Method

void HttpRequest::_checkTimeOut(clock_t timestamp) {
  if ((static_cast<double>(clock() - timestamp) / CLOCKS_PER_SEC) > PARSING_TIME_OUT) {
    _parsingState = TIME_OUT;
  }
}

void HttpRequest::_parseHeader() {
  if (_parsingState == PARSING_INIT) {
    std::string line = _storage.getLine();  // 라인이 완성되어 있지 않으면 "" 리턴

    if (line.empty()) {
      _checkTimeOut(_bodyTimeStamp);
      return;
    }
    _parseStartLine(line);
  }

  while (_parsingState != BAD_REQUEST) {
    std::string line = _storage.getLine();
    if (line.empty()) {
      _checkTimeOut(_headerTimeStamp);
      break;
    } else if (line == "\r") {
      _parsingState  = PARSING_BODY;
      _bodyTimeStamp = clock();
      break;
    } else {
      _parseHeaderField(line);
    }
  }
}

void HttpRequest::_parseStartLine(const std::string& line) {
  if (*line.rbegin() != '\r' || std::count(line.begin(), line.end(), ' ') != 2) {
    _parsingState = BAD_REQUEST;
    return;
  }

  _headerSize += line.size() + 1;
  _headerTimeStamp = clock();

  std::stringstream ss(line);
  std::string       word;

  ss >> word;
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

  ss >> _uri;  // 유효성 검사
  ss >> _httpVersion;

  if (ss.bad()) {
    _parsingState = BAD_REQUEST;
  } else {
    _parsingState = PARSING_HEADER;
  }
}

void HttpRequest::_parseHeaderField(const std::string& line) {
  if (*line.rbegin() != '\r' || line.size() + 1 + _headerSize > HTTP_MAX_HEADER_SIZE) {
    _parsingState = BAD_REQUEST;
    return;
  }

  _headerSize += line.size() + 1;
  _headerTimeStamp = clock();

  std::stringstream ss(line);
  std::string       word;

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
    ss >> word;
    if (word == "keep-alive") {
      _isKeepAlive = true;
    }
  } else if (word == "Transfer-Encoding:") {
    ss >> word;
    if (word == "chunked") {
      _isChunked     = true;
      _isBodyExisted = true;
    }
  }
}

void HttpRequest::_parseBody() {
  if (!_isBodyExisted || _parsingState == BAD_REQUEST || _parsingState == TIME_OUT) {
    _parsingState = PARSING_DONE;
    return;
  }

  if (_isChunked) {
    _parseChunk();
  } else if (_storage.remains() > _contentLength) {
    _storage.dataToBody(_body, _contentLength);
    _parsingState = PARSING_DONE;
  }
}

void HttpRequest::_parseChunk() {
  while (1) {
    if (_chunkSize == -1) {
      std::string line = _storage.getLine();
      if (line.empty()) {
        _checkTimeOut(_bodyTimeStamp);
        return;
      }
      _chunkSize = _parseChunkSize(line);
      Log::log()(true, "line", line);
      Log::log()(true, "chunkSize", _chunkSize);
    }

    if (_chunkSize == 0) {
      _parsingState = PARSING_DONE;
      _chunkSize    = -1;
      return;
    }

    if (_storage.remains() > _chunkSize) {
      _storage.dataToBody(_body, _chunkSize);
      _bodyTimeStamp = clock();
      _chunkSize     = -1;
    } else {
      break;
    }
  }
}

size_t HttpRequest::_parseChunkSize(const std::string& line) {
  char*  p;
  size_t size = std::strtol(line.c_str(), &p, 16);  // chunk 최대 크기 몇?
  if (*p != '\r') {
    _parsingState = BAD_REQUEST;
    Log::log()(LOG_LOCATION, "BAD REQUEST", ALL);
    return 0;
  }
  return size;
}