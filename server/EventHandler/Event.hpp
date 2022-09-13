#pragma once
#define Event_hpp

#include <sys/time.h>
#include <sys/types.h>

#include "CgiResponse.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

enum EventType {
  CONNECTION_REQUEST,
  HTTP_REQUEST_READABLE,
  HTTP_REQUEST_UPLOAD,
  HTTP_RESPONSE_WRITABLE,
  CGI_RESPONSE_READABLE,
  SOCKET_FLSUH
};

struct Event {
  enum EventType type;
  int            serverId;
  int            toSendFd;
  int            toRecvFd;
  int            clientFd;
  HttpRequest    httpRequest;
  CgiResponse    cgiResponse;
  HttpResponse*  httpResponse;
  timeval        timestamp;
  clock_t        baseClock;

  Event(enum EventType t, int kevent_id);
  ~Event() { delete httpResponse; }
  void initialize();
  void setDataFlow(int from_fd, int to_fd);
};
