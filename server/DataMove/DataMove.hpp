#pragma once
#include <sys/socket.h>

#include "GetLine.hpp"
#include "Log.hpp"
#include "syscall.hpp"

class DataMove {
 private:
  static bool _fail;

 public:
  template <typename T>
  static void memToFile(const T* mem, size_t size, int file_fd, size_t& moved);
  static void fileToFile(int recv_fd, int send_fd, size_t& moved, size_t to_read = PUBLIC_BUFFER_SIZE);
  static bool fail() { return _fail; }
};

template <typename T>
void DataMove::memToFile(const T* mem, size_t size, int file_fd, size_t& moved) {
  const T* pos       = mem + moved;
  int      remains   = size - moved;
  int      send_size = send(file_fd, pos, remains, 0);
  if (send_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return;
  }
  moved += send_size;
  _fail = false;
}
