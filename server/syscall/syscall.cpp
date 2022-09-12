#include "syscall.hpp"

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "Log.hpp"
#include "Router.hpp"

off_t ft::syscall::lseek(int fd, off_t offset, int whence) {
  off_t ret = ::lseek(fd, offset, whence);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    Log::log()(true, "fd", fd);
    Log::log()(true, "offset", offset);
    Log::log()(true, "whence", whence);
    throw Router::ServerSystemCallException("(SYSCALL) lseek");
  }
  return ret;
}

int ft::syscall::open(const char *path, int oflag, mode_t mode) {
  int ret = ::open(path, oflag, mode);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), ALL);
    Log::log()(true, "path", path);
    Log::log()(true, "oflag", oflag);
    Log::log()(true, "mode", mode);
    throw Router::ServerSystemCallException("(SYSCALL) open");
  }
  return ret;
}

void ft::syscall::fcntl(int fd, int cmd, int oflag) {
  int ret = ::fcntl(fd, cmd, oflag);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSystemCallException("(SYSCALL) fcntl");
  }
  return;
}