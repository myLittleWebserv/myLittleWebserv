#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <ctime>
#include <map>
#include <set>
#include <vector>

#include "Event.hpp"

#define MAX_EVENTS 2000
#define KEVENT_TIMEOUT_MILISEC 10000
#define CONNECTION_TIMEOUT_MILISEC 10000  // KEVENT_TIMEOUT_MILISEC 보다 크거나 같음.

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

  // Method
 private:
  void _checkConnectionTimeout(const timeval& base_time);
  void _updateEventsTimestamp(int num_kevents);
  void _routeEvent(Event& event);
  void _appendNewEventToChangeList(int ident, int filter, int flag, Event* event);
  void _addConnection(int listen_fd);

  // Interface
 public:
  void disableReadEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_READ, EV_DISABLE, event); }
  void disableWriteEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_WRITE, EV_DISABLE, event); }
  void enableReadEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_READ, EV_ENABLE, event); }
  void enableWriteEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_WRITE, EV_ENABLE, event); }
  void addReadEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_READ, EV_ADD, event); }
  void deleteReadEvent(int id, Event* event) { _appendNewEventToChangeList(id, EVFILT_READ, EV_DELETE, event); }
  void removeConnection(Event& event);
  void routeEvents();
  std::vector<Event*>& getRoutedEvents(int server_id);
};

#endif  // EvnetHandler_hpp
