#include "DataMove.hpp"

#include "Storage.hpp"

bool DataMove::_fail = false;

void DataMove::fileToFile(int recv_fd, int send_fd, size_t& moved, size_t to_read) {
  if (recv_fd == -1 || send_fd == -1)
    return;
  int recv_size = read(recv_fd, GetLine::publicBuffer, to_read);
  int send_size = write(send_fd, GetLine::publicBuffer, recv_size);

  if (send_size == -1 || recv_size == -1) {
    _fail = true;
    Log::log()(LOG_LOCATION, "errno : " + std::string(strerror(errno)), INFILE);
    return;
  }
  ft::syscall::lseek(recv_fd, static_cast<off_t>((recv_size - send_size) * -1), SEEK_CUR);
  moved += send_size;
  _fail = false;
}
