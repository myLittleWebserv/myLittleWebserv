#include "syscall.hpp"

#include <string.h>
#include <unistd.h>

#include "Log.hpp"
#include "Router.hpp"

off_t ft::syscall::lseek(int fd, off_t offset, int whence) {
  off_t ret = ::lseek(fd, offset, whence);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSystemCallException();
  }
  return ret;
}