#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#define BUFFER_SIZE 8000

void readFile(std::stringstream &storage) {
  std::ifstream infile("test/HttpRequest.infile");
  if (!infile.is_open()) {
    std::cerr << "file open failed" << std::endl;
    std::exit(1);
  }

  std::string line;
  while (!infile.eof()) {
    std::getline(infile, line);
    storage << line << "\r\n";
  }
}

int connectWithServer(int port) {
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  int ret;

  sockaddr_in server_addr = {.sin_family = AF_INET,
                             .sin_port   = htons(port),
                             .sin_addr   = {.s_addr = inet_addr("127.0.0.1")},
                             .sin_zero   = {
                                   0,
                             }};

  ret = connect(client_fd, (sockaddr *)&server_addr, sizeof(server_addr));
  if (ret == -1) {
    std::cerr << "Client: connection fail" << std::endl;
    std::exit(1);
  }

  std::cout << "Client: connected!" << std::endl;
  return client_fd;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "plz input only port!" << std::endl;
    return 1;
  }

  std::stringstream storage;
  readFile(storage);

  std::cout << "input\n" << storage.str() << std::endl;

  int fd = connectWithServer(std::atoi(argv[1]));

  send(fd, storage.str().c_str(), storage.str().size(), 0);

  unsigned char buf[BUFFER_SIZE];

  int recv_size = recv(fd, buf, BUFFER_SIZE, 0);
  std::cout << "\noutput" << std::endl;
  write(1, buf, recv_size);
}
