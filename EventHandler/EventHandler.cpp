#include "EventHandler.hpp"

#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_EVENTS 20
#define TIME_OUT_MICRO 10000

EventHandler::EventHandler(Router* router, int kqfd) : _router(router), _kQueue(kqfd) {
  _timeOut.tv_sec  = TIME_OUT_MICRO / 1000;
  _timeOut.tv_nsec = TIME_OUT_MICRO % 1000 * 1000 * 1000;
}

std::vector<Event>& EventHandler::getRoutedEvents(int server_id) { return _routedEvents[server_id]; }

void EventHandler::appendNewEventToChangeList(struct kevent& kevent) { _changeList.push_back(kevent); }

void EventHandler::addConnection(int listen_fd) {
  sockaddr_in   addr;
  socklen_t     alen;
  int           client_fd = accept(listen_fd, (struct sockaddr*)&addr, &alen);  // addr 버리나?
  struct kevent kevent;

  EV_SET(&kevent, client_fd, EVFILT_READ, EV_ADD, 0, 0, new Event(HTTP_REQUEST_READABLE, client_fd));
  _changeList.push_back(kevent);
}

void EventHandler::removeConnection(Event& event) {
  struct kevent kevent;
  EV_SET(&kevent, event.keventId, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
  _changeList.push_back(kevent);
  EV_SET(&kevent, event.keventId, EVFILT_READ, EV_DISABLE, 0, 0, NULL);
  _changeList.push_back(kevent);
  close(event.keventId);
  delete &event;
}

// kevent udata shallow copy?

void EventHandler::routeEvents() {
  _routedEvents.clear();
  int num_kevents = kevent(_kQueue, _changeList.data(), _changeList.size(), _keventList.data(), MAX_EVENTS, &_timeOut);

  for (int i = 0; i < num_kevents; ++i) {
    Event& event  = *(Event*)_keventList[i].udata;
    int    filter = _keventList[i].filter;
    if (filter == EVFILT_WRITE) {  //   event.type == HTTP_REPONSE_WRITABLE
      _routedEvents[event.serverId].push_back(event);
    } else if (filter == EVFILT_READ && event.type == CONNECTION_REQUEST) {
      addConnection(event.keventId);
    } else if (filter == EVFILT_READ && event.type == HTTP_REQUEST_READABLE) {
      event.httpRequest->add(event.keventId);
      if (event.httpRequest->isEnd()) {
        event.serverId = _router->findServerId(*event.httpRequest);
        _routedEvents[event.serverId].push_back(event);
      }
    } else if (filter == EVFILT_READ && event.type == CGI_RESPONSE_READABLE) {
      event.cgiResponse->add(event.keventId);
      if (event.cgiResponse->isEnd())
        _routedEvents[event.serverId].push_back(event);
    }
  }
}
