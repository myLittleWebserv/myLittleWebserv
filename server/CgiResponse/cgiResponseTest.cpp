#include <fstream>

#include "CgiResponse.hpp"

int main() {
  CgiResponse cgiResponse;

  std::ifstream     file("cgi_output");
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string cgi_result = buffer.str();
  cgiResponse._parseCgiResponse(cgi_result);
  return 0;
}