#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <map>
#include <vector>

#include "Event.hpp"

class EventHandler {
  // Member Variable
 private:
  int                                _kQueue;
  std::vector<struct kevent>         _changeList;
  std::vector<struct kevent>         _keventList;
  std::map<int, std::vector<Event> > _routedEvents;
  // Interface
 public:
  void                addConnection(int listen_fd);
  void                appendNewEventToChangeList(struct kevent& kevent);
  void                removeConnection(Event& event);
  void                routeEvents();
  std::vector<Event>& getRoutedEvents(int server_id);
};

#endif  // EvnetHandler_hpp
