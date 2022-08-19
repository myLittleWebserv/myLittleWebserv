#if !defined(EvnetHandler_hpp)
#define EvnetHandler_hpp

#include <sys/event.h>

#include <map>
#include <vector>

class EventHandler {
  // Member Variable
 private:
  int                                _kQueue;
  std::vector<struct kevent>         _changeList;
  std::vector<struct kevent>         _keventList;
  std::map<int, std::vector<Event> > _routedEvents;
  // Interface
 public:
  void               addConnection();
  void               appendNewEventToChangeList(struct kevent);
  void               removeConnection(struct kevent);
  void               routeEvents();
  std::vector<Event> getRoutedEvents(int server_id);
}

#endif  // EvnetHandler_hpp
