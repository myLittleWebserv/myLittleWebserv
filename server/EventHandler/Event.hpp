#if !defined(Event_hpp)
#define Event_hpp

#include <sys/time.h>
#include <sys/types.h>

#include "CgiResponse.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

enum EventType { CONNECTION_REQUEST, HTTP_REQUEST_READABLE, HTTP_RESPONSE_WRITABLE, CGI_RESPONSE_READABLE };

struct Event {
  enum EventType type;
  int            keventId;
  int            serverId;
  int            clientFd;
  pid_t          pid;
  HttpRequest    httpRequest;
  CgiResponse    cgiResponse;
  HttpResponse*  httpResponse;
  timeval        timestamp;

  Event(enum EventType t, int kevent_id)
      : type(t),
        keventId(kevent_id),
        serverId(-1),
        clientFd(kevent_id),
        pid(-1),
        httpRequest(),
        cgiResponse(),
        httpResponse(NULL),
        timestamp() {
    gettimeofday(&timestamp, NULL);
  }
  ~Event() { delete httpResponse; }
  void initialize();
};

#endif  // Event_hpp
