#include "Event.hpp"

#include <sys/socket.h>

#include "FileManager.hpp"
#include "Log.hpp"

Event::Event(EventType t, int kevent_id)
    : type(t),
      serverId(-1),
      toSendFd(kevent_id),
      toRecvFd(kevent_id),
      clientFd(kevent_id),
      httpRequest(),
      cgiResponse(),
      httpResponse(NULL),
      timestamp(),
      baseClock(clock()) {
  gettimeofday(&timestamp, NULL);
}

void Event::initialize() {
  if (cgiResponse.pid() != -1) {
    // FileManager::removeTempFileByKey(clientFd);
  }

  type     = HTTP_REQUEST_READABLE;
  toSendFd = clientFd;
  toRecvFd = clientFd;
  serverId = -1;
  httpRequest.initialize();
  cgiResponse.initialize();
  delete httpResponse;
  httpResponse = NULL;
  Log::log()(LOG_LOCATION, "(init) event", INFILE);
}

void Event::setDataFlow(int from_fd, int to_fd) {
  toRecvFd = from_fd;
  toSendFd = to_fd;
}
