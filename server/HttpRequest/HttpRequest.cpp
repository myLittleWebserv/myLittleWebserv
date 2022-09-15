
#include "HttpRequest.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "Event.hpp"
#include "Log.hpp"
#include "Storage.hpp"
#include "syscall.hpp"
// Interface

void HttpRequest::initialize() {
  _parsingState = HTTP_PARSING_INIT;
  _storage.clear();
  _headerSize = 0;
  _chunkSize  = -1;
  ft::syscall::gettimeofday(&_timestamp, NULL);
  _isBodyExisted       = false;
  _isChunked           = false;
  _isKeepAlive         = true;
  _secretHeaderForTest = 0;
  _uploadedSize        = 0;
  _uploadedTotalSize   = 0;
  _method              = NOT_IMPL;
  _uri                 = "";
  _contentLength       = 0;
  _contentType         = "";
  _hostName            = "";
  _hostPort            = HTTP_DEFAULT_PORT;
}

bool HttpRequest::isParsingEnd() {
  return _parsingState == HTTP_UPLOADING_CHUNK_INIT || _parsingState == HTTP_UPLOADING_INIT ||
         _parsingState == HTTP_PARSING_BAD_REQUEST || _parsingState == HTTP_PARSING_TIME_OUT;
}

bool HttpRequest::isRecvError() {
  return _storage.state() == CONNECTION_CLOSED || _parsingState == HTTP_PARSING_CONNECTION_CLOSED ||
         _parsingState == HTTP_PARSING_TIME_OUT;
}

bool HttpRequest::isUploadEnd() { return _parsingState == HTTP_UPLOADING_DONE; }

bool HttpRequest::isCgi(const std::string& ext) {
  std::string::size_type ext_delim = _uri.rfind('.');
  if (std::string::npos == ext_delim || _uri.substr(ext_delim) != ext) {
    return false;
  }
  return true;
}

void HttpRequest::uploadRequest(int send_fd, clock_t base_clock) {
  (void)base_clock;
  std::string line;
  ssize_t     moved;

  switch (_parsingState) {
    case HTTP_UPLOADING_CHUNK_INIT:
    case HTTP_UPLOADING_READ_LINE:
      line = _storage.getLine();
      if (_storage.state() == CONNECTION_CLOSED) {
        _parsingState = HTTP_PARSING_CONNECTION_CLOSED;
        break;
      }
      if (line == "\r" && _chunkSize == 0) {
        _parsingState = HTTP_UPLOADING_DONE;
        break;
      }
      if (line == "\r") {
        break;
      }
      _parsingState = HTTP_UPLOADING_READ_LINE;

    case HTTP_UPLOADING_CHUNK_SIZE:
      _chunkSize = _parseChunkSize(line);
      if (line.empty() || isParsingEnd())
        break;
      ft::syscall::gettimeofday(&_timestamp, NULL);
      if (_chunkSize == 0) {
        _parsingState = HTTP_UPLOADING_READ_LINE;
        break;
      }
      _uploadedSize = 0;
      _parsingState = HTTP_UPLOADING_INIT;

    case HTTP_UPLOADING_INIT:
      moved = _storage.memToFile(send_fd, _chunkSize - _uploadedSize);
      if (moved == -1) {
        _parsingState = HTTP_PARSING_CONNECTION_CLOSED;
        break;
      }
      ft::syscall::gettimeofday(&_timestamp, NULL);
      _uploadedSize += moved;
      _uploadedTotalSize += moved;
      if (_isChunked && _chunkSize == _uploadedSize) {
        _parsingState = HTTP_UPLOADING_READ_LINE;
        Log::log()(_uploadedTotalSize > 50000000, "_uploadedTotalSize", _uploadedTotalSize);

      } else if (!_isChunked && _chunkSize == _uploadedSize) {
        Log::log()(_uploadedTotalSize > 50000000, "_uploadedTotalSize", _uploadedTotalSize);
        _parsingState = HTTP_UPLOADING_DONE;
      } else
        _parsingState = HTTP_UPLOADING_INIT;

    default:
      break;
  }
  _checkTimeOut();
}

void HttpRequest::parseRequest(int recv_fd, clock_t base_clock) {
  _storage.sockToMem(recv_fd);
  if (_storage.state() != RECEIVE_DONE) {
    return;
  }

  (void)base_clock;
  std::string line;

  switch (_parsingState) {
    case HTTP_PARSING_INIT:
      line = _storage.getLine();
      _parseStartLine(line);
      if (line.empty() || isParsingEnd())
        break;
      _parsingState = HTTP_PARSING_HEADER;

    case HTTP_PARSING_HEADER:
      line = _storage.getLine();
      while (_parseHeaderField(line)) {
        line = _storage.getLine();
      }
      if (line.empty() || isParsingEnd())
        break;
      _bodyFirst    = _storage.readPos();
      _parsingState = HTTP_PARSING_BODY;

    case HTTP_PARSING_BODY:
      if (_isChunked && _parseChunk()) {
        _storage.setReadPos(_bodyFirst);
        _parsingState = HTTP_UPLOADING_CHUNK_INIT;
        Log::log()(_storage.remains() > 50000000, "_storage.remains", _storage.remains());
        Log::log()(_storage.remains() > 50000000, "_bodyFirst", _bodyFirst);
      } else if (!_isChunked && _storage.remains() >= _contentLength) {
        _chunkSize    = _contentLength;
        _parsingState = HTTP_UPLOADING_INIT;
      }
      ft::syscall::gettimeofday(&_timestamp, NULL);

    default:
      break;
  }
  _checkTimeOut();
}

// Method

void HttpRequest::_checkTimeOut() {
  timeval current;
  ft::syscall::(&current, NULL);
  int interval;
  interval = (current.tv_sec - _timestamp.tv_sec) * 1000 + (current.tv_usec - _timestamp.tv_usec) / 1000;

  if (interval >= TIME_OUT_HTTP_PARSING) {
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
  ft::syscall::gettimeofday(&_timestamp, NULL);

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
  ft::syscall::gettimeofday(&_timestamp, NULL);

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
    // Log::log()(true, "header-value", word);
  } else if (word == "Transfer-Encoding:") {
    std::getline(ss, word, '\r');
    if (word == "chunked") {
      _isChunked     = true;
      _isBodyExisted = true;
    }
  } else if (word == "X-Secret-Header-For-Test:") {
    ss >> _secretHeaderForTest;
  } else {
    // Log::log()(true, "header-field", word);
    ss >> word;
    // Log::log()(true, "header-value", word);
  }
  return true;
}

bool HttpRequest::_parseChunk() {
  while (1) {
    if (_chunkSize == -1) {
      std::string line = _storage.getLine();
      if (line.empty()) {
        return false;
      }
      _chunkSize = _parseChunkSize(line);
    }

    if (_chunkSize == 0) {
      std::string line = _storage.getLine();
      if (line.empty()) {
        return false;
      }
      _chunkSize = -1;
      return true;
    }

    if (_storage.remains() > _chunkSize + 2) {  // jump "\r\n" .. ?
      _storage.moveReadPos(_chunkSize + 2);
      _chunkSize = -1;
    } else {
      break;
    }
  }
  ft::syscall::gettimeofday(&_timestamp, NULL);
  return false;
}

ssize_t HttpRequest::_parseChunkSize(const std::string& line) {
  if (line.empty()) {
    return -1;
  }
  char*   p;
  ssize_t size = std::strtol(line.c_str(), &p, 16);  // chunk 최대 크기 몇?
  if (*p != '\r') {
    _parsingState = HTTP_PARSING_BAD_REQUEST;
    Log::log()(LOG_LOCATION, "BAD_REQUEST", INFILE);
    Log::log()(true, "line", line, INFILE);
    return -1;
  }
  return size;
}
