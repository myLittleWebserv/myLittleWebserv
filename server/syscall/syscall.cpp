#include "syscall.hpp"

#include <fcntl.h>
#include <string.h>
#include <sys/event.h>
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
  Log::log()(true, "file opend fd", ret);
  return ret;
}

void ft::syscall::fcntl(int fd, int cmd, int oflag, const std::exception &e) {
  if (fd == -1) {
    return;
  }
  int ret = ::fcntl(fd, cmd, oflag);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    Log::log()(true, "fd", fd);
    throw e;
  }
  return;
}
void ft::syscall::unlink(const char *file_path) {
  int ret = ::unlink(file_path);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSystemCallException("(SYSCALL) unlink");
  }
  return;
}

void ft::syscall::close(int fd) {
  int ret = ::close(fd);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    Log::log()(true, "fd", fd);
    throw Router::ServerSystemCallException("(SYSCALL) close");
  }
  Log::log()(true, "closed fd", fd);
  return;
}

int ft::syscall::accept(int fd, ::sockaddr *addr, ::socklen_t *alen) {
  int ret = ::accept(fd, addr, alen);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    Log::log()("Server Socket fd", fd, INFILE);
    Log::log()("Client Socket fd", ret, INFILE);
    throw Router::ServerSystemCallException("(SYSCALL) accept");
  }
  Log::log()("Server Socket fd", fd, INFILE);
  Log::log()("Client Socket fd", ret, INFILE);
  return ret;
}

int ft::syscall::kqueue() {
  int ret = ::kqueue();
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSystemCallException("(SYSCALL) kqueue");
  }
  return ret;
}

int ft::syscall::kevent(int fd, struct kevent *changelist, int nchanges, struct kevent *eventlist, int nevents,
                        timespec *timeout) {
  int ret = ::kevent(fd, changelist, nchanges, eventlist, nevents, timeout);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSystemCallException("(SYSCALL) kevent");
  }
  return ret;
}

void ft::syscall::listen(int fd, int back_log) {
  int ret = ::listen(fd, back_log);
  if (ret == -1) {
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)));
    throw Router::ServerSocketInitException();
  }
}

void ft::syscall::bind(int fd, ::sockaddr *addr, socklen_t len) {
  int ret = ::bind(fd, addr, len);
  if (ret < 0) {
    throw Router::ServerSocketInitException();
  }
}