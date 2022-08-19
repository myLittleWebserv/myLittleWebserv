#if !defined(Event_hpp)
#define Event_hpp

enum EventType { CONNECTION_REQUEST, HTTP_REQUEST_READABLE, HTTP_RESPONSE_WRITABLE, CGI_RESPONSE_WRITABLE };

class HttpRequest;
class CgiResponse;

struct Event {
  enum EventType type;
  int            serverId;
  HttpRequest*   httpRequest;
  CgiResponse*   cgiResponse;
  int            keventId;

  Event(enum EventType t, int kevent_id) : type(t), keventId(kevent_id) {}
};

#endif  // Event_hpp
