#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

extern char **environ;

int main(int argc, char *argv[], char *env[]) {
  (void)argc;
  char **cgi_argv;

  setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  setenv("REQUEST_METHOD", "POST", 1);
  setenv("PATH_INFO", "/", 1);

  cgi_argv    = (char **)malloc(sizeof(char *) * (2 + 1));
  cgi_argv[0] = "cgi_tester";
  cgi_argv[1] = "";
  cgi_argv[2] = NULL;
  int _pipe[2];
  pipe(_pipe);
  int pid = fork();
  int fd  = open("cgi_input_empty", O_RDONLY);
  if (pid == 0) {
    dup2(fd, 0);
    dup2(_pipe[1], 1);
    close(_pipe[0]);
    execve(cgi_argv[0], cgi_argv, environ);
  } else {
    waitpid(pid, NULL, 0);
    close(_pipe[1]);
    int  red;
    char buf[1024];
    red         = read(_pipe[0], buf, 1024);
    int outfile = open("cgi_output_empty", O_WRONLY | O_CREAT, 0644);
    write(outfile, buf, red);
  }
}
