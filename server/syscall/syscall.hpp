#pragma once
#include <unistd.h>

namespace ft {

namespace syscall {
off_t lseek(int fd, off_t offset, int whence);
}
}  // namespace ft