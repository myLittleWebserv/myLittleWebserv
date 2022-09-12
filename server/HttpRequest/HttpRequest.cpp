
#include "HttpRequest.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "DataMove.hpp"
#include "Event.hpp"
#include "GetLine.hpp"
#include "Log.hpp"
#include "Storage.hpp"
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
  _uploadedSize        = 0;
  _uploadedTotalSize   = 0;
  _storage.preserveRemains();
}

bool HttpRequest::isParsingEnd() {
  return _parsingState == HTTP_PARSING_CHUNK_INIT || _parsingState == HTTP_UPLOADING_INIT ||
         _parsingState == HTTP_PARSING_BAD_REQUEST || _parsingState == HTTP_PARSING_TIME_OUT;
}

bool HttpRequest::isRecvError() {
  return _storage.fail() || _parsingState == HTTP_PARSING_CONNECTION_CLOSED || _parsingState == HTTP_PARSING_TIME_OUT;
}

bool HttpRequest::isUploadEnd() { return _parsingState == HTTP_UPLOADING_DONE; }

bool HttpRequest::isCgi(const std::string& ext) {
  std::string::size_type ext_delim = _uri.rfind('.');
  if (std::string::npos == ext_delim || _uri.substr(ext_delim) != ext) {
    return false;
  }
  return true;
}

void HttpRequest::uploadRequest(int recv_fd, int send_fd, clock_t base_clock) {
  (void)base_clock;
  std::string line;
  ssize_t     moved;

  Log::log()(true, "(START) uploadRequest _parsingState", _parsingState);

  switch (_parsingState) {
    case HTTP_PARSING_CHUNK_INIT:
    case HTTP_PARSING_READ_LINE:
      line = _storage.getLine(recv_fd);
      if (_storage.fail()) {
        _parsingState = HTTP_PARSING_CONNECTION_CLOSED;
        break;
      }
      if (line == "\r" && _chunkSize == 0) {
        Log::log()(LOG_LOCATION, "(DONE) HTTP_UPLOADING_DONE");
        _parsingState = HTTP_UPLOADING_DONE;
        break;
      }
      if (line == "\r") {
        break;
      }
      _parsingState = HTTP_PARSING_READ_LINE;
      Log::log()(LOG_LOCATION, "(DONE) HTTP_PARSING_READ_LINE");

    case HTTP_PARSING_CHUNK_SIZE:
      _chunkSize = _parseChunkSize(line);
      if (line.empty() || isParsingEnd())
        break;
      if (_chunkSize == 0) {
        _parsingState = HTTP_PARSING_READ_LINE;
        break;
      }
      _uploadedSize = 0;
      _parsingState = HTTP_UPLOADING_INIT;
      Log::log()(LOG_LOCATION, "(DONE) HTTP_PARSING_CHUNK_SIZE");

    case HTTP_UPLOADING_INIT:
      moved = _storage.dataToFile(recv_fd, send_fd, _chunkSize - _uploadedSize);
      if (moved == -1) {
        _parsingState = HTTP_PARSING_CONNECTION_CLOSED;
        break;
      }
      _uploadedSize += moved;
      _uploadedTotalSize += moved;
      if (_isChunked && _chunkSize == _uploadedSize)
        _parsingState = HTTP_PARSING_READ_LINE;
      else if (!_isChunked && _chunkSize == _uploadedSize)
        _parsingState = HTTP_UPLOADING_DONE;
      else
        _parsingState = HTTP_UPLOADING_INIT;

    default:
      break;
  }
  Log::log()(true, "(END) uploadRequest _parsingState", _parsingState);
  _checkTimeOut();
}

void HttpRequest::parseRequest(int recv_fd, clock_t base_clock) {
  (void)base_clock;
  std::string line;

  switch (_parsingState) {
    case HTTP_PARSING_INIT:
      line = _storage.getLine(recv_fd);
      _parseStartLine(line);
      if (line.empty() || isParsingEnd())
        break;
      _parsingState = HTTP_PARSING_HEADER;

    case HTTP_PARSING_HEADER:
      line = _storage.getLine(recv_fd);
      while (_parseHeaderField(line)) {
        line = _storage.getLine(recv_fd);
      }
      if (line.empty() || isParsingEnd())
        break;
      _parsingState = HTTP_PARSING_BODY;

    case HTTP_PARSING_BODY:
      _fileFd = recv_fd;
      _storage.preserveRemains();
      if (!_isChunked) {
        _chunkSize    = _contentLength;
        _parsingState = HTTP_UPLOADING_INIT;
        break;
      }

    default:
      _parsingState = HTTP_PARSING_CHUNK_INIT;
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

size_t HttpRequest::_parseChunkSize(const std::string& line) {
  if (line.empty()) {
    return 0;
  }
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
