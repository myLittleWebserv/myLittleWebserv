#pragma once
#include <unistd.h>

namespace ft {

namespace syscall {
off_t lseek(int fd, off_t offset, int whence);
int   open(const char *path, int oflag, mode_t mode = 666);
void  fcntl(int fd, int cmd, int oflag);
}  // namespace syscall
}  // namespace ft