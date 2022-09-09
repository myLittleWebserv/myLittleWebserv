#include "Event.hpp"

void Event::initialize() {
  type     = HTTP_REQUEST_READABLE;
  keventId = toSendFd;
  serverId = -1;
  fileFd   = -1;
  httpRequest.initialize();
  cgiResponse.initialize();
  delete httpResponse;
  httpResponse = NULL;
  Log::log()(LOG_LOCATION, "(init) event", INFILE);
}
