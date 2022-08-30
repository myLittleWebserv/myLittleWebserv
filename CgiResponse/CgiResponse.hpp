#ifndef CGIRESPONSE_HPP

#include <fcntl.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class CgiResponse {
 private:
  bool        _isEnd;
  int         _statusCode;
  std::string _statusMessage;
  std::string _contentType;
  std::string _body;

  std::vector<std::string> _split(const std::string& str, const std::string& delimiter);
  void                     _printCgiResponse();

  // Constructor
 public:
  CgiResponse();

  // Interface
 public:
  bool        isEnd();
  void        readCgiResult(int fd);
  void        setCgiEnd();
  int         getStatusCode();
  std::string getStatusMessage();
  std::string getContentType();
  std::string getBody();

  void _parseCgiResponse(std::string& cgi_result);
};

#endif
