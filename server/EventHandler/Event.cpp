#include "Event.hpp"

#include <sys/socket.h>

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
  // switch (httpResponse->statusCode()) {
  //   case STATUS_BAD_REQUEST:
  //   case STATUS_PAYMENT_REQUIRED:
  //   // case STATUS_NOT_FOUND:
  //   case STATUS_METHOD_NOT_ALLOWED:
  //   case STATUS_PAYLOAD_TOO_LARGE:
  //     while (recv(clientFd, Storage::publicBuffer, PUBLIC_BUFFER_SIZE, 0) > 0)
  //       ;
  //     httpRequest.storage().clear();
  //     break;
  //   default:
  //     break;
  // }
  delete httpResponse;
  httpResponse = NULL;
  Log::log()(LOG_LOCATION, "(init) event", INFILE);
}
