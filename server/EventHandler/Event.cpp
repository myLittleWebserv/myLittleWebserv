#include "Event.hpp"

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
