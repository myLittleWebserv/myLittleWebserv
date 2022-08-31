#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <ctime>
#include <map>
#include <set>
#include <vector>

#include "Event.hpp"

#define MAX_EVENTS 200
#define TIME_OUT_MICRO 10000
#define TIME_OUT_SEC 10

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
  void                 _checkClientTimeOut();
  std::vector<Event*>& getRoutedEvents(int server_id);
};

#endif  // EvnetHandler_hpp
