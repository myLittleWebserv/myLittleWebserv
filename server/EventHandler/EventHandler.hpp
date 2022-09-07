#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <ctime>
#include <map>
#include <set>
#include <vector>

#include "Event.hpp"

#define MAX_EVENTS 2000
#define KEVENT_TIMEOUT_MILISEC 50
#define CONNECTION_TIMEOUT_MILISEC 50  // KEVENT_TIMEOUT_MILISEC 보다 크거나 같음.

class Router;

class EventHandler {
  // Member Variable
 private:
  const Router&                       _router;
  int                                 _kQueue;
  std::vector<struct kevent>          _changeList;
  struct kevent                       _keventList[MAX_EVENTS];
  std::map<int, std::vector<Event*> > _routedEvents;
  std::set<Event*>                    _eventSet;
  timespec                            _timeOut;

  // Constructor
 public:
  EventHandler(const Router& router);
  // Interface
 public:
  void                 addConnection(int listen_fd);
  void                 appendNewEventToChangeList(int ident, int filter, int flag, Event* event);
  void                 removeConnection(Event& event);
  void                 routeEvents();
  void                 _checkUnusedFd(const timeval& base_time);
  std::vector<Event*>& getRoutedEvents(int server_id);
};

#endif  // EvnetHandler_hpp
