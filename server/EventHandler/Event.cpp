#include "Event.hpp"

void Event::initialize() {
  type = HTTP_REQUEST_READABLE;
  httpRequest.initialize();
  // cgiResponse.initialize();
  delete httpResponse;
  httpResponse = NULL;
  keventId     = clientFd;
  // Log::log()(LOG_LOCATION, "(FREE) event.httpResponse removed", ALL);
}
