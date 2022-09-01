#include "CgiResponse.hpp"



int main() {
  CgiResponse response;

  int fd = open("cgi_output", O_RDONLY);
  response.readCgiResult(fd);
  std::cout << response.CgiResponseResultString();
  return 0;
}