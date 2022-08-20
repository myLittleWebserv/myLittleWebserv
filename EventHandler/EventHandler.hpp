#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <map>
#include <vector>

#include "Event.hpp"

class Router;

class EventHandler {
  // Member Variable
 private:
  Router*                             _router;
  int                                 _kQueue;
  std::vector<struct kevent>          _changeList;
  std::vector<struct kevent>          _keventList;
  std::map<int, std::vector<Event*> > _routedEvents;
  timespec                            _timeOut;

  // Constructor
 public:
  EventHandler(Router* router);
  // Interface
 public:
  void                 addConnection(int listen_fd);
  void                 appendNewEventToChangeList(int ident, int filter, int flag, Event* event);
  void                 removeConnection(Event& event);
  void                 routeEvents();
  std::vector<Event*>& getRoutedEvents(int server_id);
};

#endif  // EvnetHandler_hpp
