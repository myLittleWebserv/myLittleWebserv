#if !defined(Event_hpp)
#define Event_hpp

#include <sys/time.h>
#include <sys/types.h>

#include "CgiResponse.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

struct LocationInfo;

enum EventType {
  CONNECTION_REQUEST,
  HTTP_REQUEST_READABLE,
  HTTP_RESPONSE_WRITABLE,
  CGI_REQUEST_WRITABLE,
  CGI_RESPONSE_READABLE,
  CGI
};

struct Event {
  enum EventType type;
  int            keventId;
  int            serverId;
  int            clientFd;
  int            pipeFd;
  pid_t          pid;
  HttpRequest    httpRequest;
  CgiResponse    cgiResponse;
  HttpResponse*  httpResponse;
  LocationInfo*  locationInfo;
  timeval        timestamp;
  clock_t        baseClock;

  Event(enum EventType t, int kevent_id)
      : type(t),
        keventId(kevent_id),
        serverId(-1),
        clientFd(kevent_id),
        pipeFd(-1),
        pid(-1),
        httpRequest(),
        cgiResponse(),
        httpResponse(NULL),
        locationInfo(NULL),
        timestamp(),
        baseClock(clock()) {
    gettimeofday(&timestamp, NULL);
  }
  ~Event() { delete httpResponse; }
  void initialize();
};

#endif  // Event_hpp
