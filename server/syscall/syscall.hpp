#pragma once
#include <sys/socket.h>
#include <unistd.h>

#include <exception>
struct kevent;

namespace ft {

namespace syscall {
off_t lseek(int fd, off_t offset, int whence);
int   open(const char* path, int oflag, mode_t mode = 0666);
void  fcntl(int fd, int cmd, int oflag, const std::exception& e);
void  unlink(const char* file_path);
void  close(int fd);
int   accept(int fd, ::sockaddr* addr, ::socklen_t* len);
void  listen(int fd, int back_log);
void  bind(int fd, ::sockaddr* addr, socklen_t len);
int   kqueue();
int   kevent(int fd, struct kevent* changelist, int nchanges, struct kevent* eventlist, int nevents, timespec* timeout);
int   socket(int domain, int type, int protocol);
void  gettimeofday(timeval* time, void* tzp);
void  setsockopt(int fd, int level, int option_name, void* opt_value, socklen_t len);
}  // namespace syscall
}  // namespace ft