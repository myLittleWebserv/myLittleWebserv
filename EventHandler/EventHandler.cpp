#include "EventHandler.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Log.hpp"
#include "Router.hpp"

EventHandler::EventHandler(const Router& router) : _router(router) {
  _kQueue          = kqueue();
  _timeOut.tv_sec  = TIME_OUT_MICRO / 1000;
  _timeOut.tv_nsec = TIME_OUT_MICRO % 1000 * 1000 * 1000;
}

std::vector<Event*>& EventHandler::getRoutedEvents(int server_id) { return _routedEvents[server_id]; }

void EventHandler::appendNewEventToChangeList(int ident, int filter, int flag, Event* event) {
  struct kevent kevent;
  EV_SET(&kevent, ident, filter, flag, 0, 0, event);
  _changeList.push_back(kevent);
}

void EventHandler::removeConnection(Event& event) { close(event.keventId); }

void EventHandler::addConnection(int listen_fd) {
  sockaddr_in addr;
  socklen_t   alen;
  int         client_fd = accept(listen_fd, (struct sockaddr*)&addr, &alen);  // addr 버리나?

  Log::log().condition(client_fd != -1, LOG_LOCATION, "(connect) accpeted", "(connect) not accepted", ALL);
  Log::log().getLogStream() << "Server Socket fd : " << listen_fd << " Client fd : " << client_fd << std::endl;

  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  Event* event = new Event(HTTP_REQUEST_READABLE, client_fd);

  appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD, event);
  appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD | EV_DISABLE, event);
}

void EventHandler::routeEvents() {
  int num_kevents = kevent(_kQueue, _changeList.data(), _changeList.size(), _keventList, MAX_EVENTS, &_timeOut);
  _changeList.clear();
  _routedEvents.clear();

  if (num_kevents == 0) {
    Log::log()(LOG_LOCATION, "Wating event...", CONSOLE);
  }

  for (int i = 0; i < num_kevents; ++i) {
    Event& event  = *(Event*)_keventList[i].udata;
    int    filter = _keventList[i].filter;

    if (filter == EVFILT_WRITE) {  //   event.type == HTTP_REPONSE_WRITABLE
      _routedEvents[event.serverId].push_back(&event);
      Log::log()(LOG_LOCATION, "(event routed) Http Response Writable", ALL);
    } else if (filter == EVFILT_READ && event.type == CONNECTION_REQUEST) {
      addConnection(event.keventId);
    } else if (filter == EVFILT_READ && event.type == HTTP_REQUEST_READABLE) {
      event.httpRequest.storeChunk(event.keventId);
      if (event.httpRequest.isEnd()) {
        event.serverId = _router.findServerId(event.httpRequest);
        _routedEvents[event.serverId].push_back(&event);
        appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, NULL);
        Log::log()(LOG_LOCATION, "(event routed) Http Request Readable", ALL);
      }
    } else if (filter == EVFILT_READ && event.type == CGI_RESPONSE_READABLE) {
      event.cgiResponse.storeChunk(event.keventId);
      if (event.cgiResponse.isEnd()) {
        _routedEvents[event.serverId].push_back(&event);
        appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, NULL);
        Log::log()(LOG_LOCATION, "(event routed) Cgi Reponse Readable", ALL);
      }
    }
  }
}
