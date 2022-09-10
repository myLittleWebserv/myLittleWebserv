#include "Event.hpp"

#include "Log.hpp"

Event::Event(EventType t, int kevent_id)
    : type(t),
      keventId(kevent_id),
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
  type     = HTTP_REQUEST_READABLE;
  keventId = clientFd;
  toSendFd = clientFd;
  toRecvFd = clientFd;
  serverId = -1;
  httpRequest.initialize();
  cgiResponse.initialize();
  delete httpResponse;
  httpResponse = NULL;
  Log::log()(LOG_LOCATION, "(init) event", INFILE);
}
