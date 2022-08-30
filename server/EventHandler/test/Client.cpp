#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#define BUFFER_SIZE 8000

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "plz input only port!" << std::endl;
    return 1;
  }

  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  int ret;

  sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr));
  server_addr.sin_family      = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_addr.sin_port        = htons(std::atoi(argv[1]));

  ret = connect(client_fd, (sockaddr *)&server_addr, sizeof(server_addr));
  if (ret == -1) {
    std::cout << "Client: connection fail" << std::endl;
    return 1;
  }
  std::cout << "Client: connected!" << std::endl;

  std::string line;
  char        buffer[BUFFER_SIZE * 100];
  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  while (1) {
    bzero(buffer, BUFFER_SIZE * 100);
    std::cout << "input : ";
    std::getline(std::cin, line);
    if (line == "input end")
      break;
    if (line == "header end")
      line = "\r\n";
    else
      line += "\r\n";
    // line = "GET / HTTP/1.1\r\n\r\n";
    std::cout << "Client: send to server: '" << line << "' length: " << line.length() << std::endl;
    send(client_fd, line.c_str(), line.length(), 0);
    int recv_size = recv(client_fd, buffer, BUFFER_SIZE * 100, 0);
    printf("ret : %d\n", recv_size);
  }
  write(1, "Client: receive from server: ", strlen("Client: receive from server: "));
  write(1, buffer, BUFFER_SIZE * 100);
  write(1, "\n", 1);
}