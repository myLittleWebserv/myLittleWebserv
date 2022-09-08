#include "EventHandler.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "FileManager.hpp"
#include "Log.hpp"
#include "Router.hpp"

EventHandler::EventHandler(const Router& router) : _router(router) {
  _kQueue          = kqueue();
  _timeOut.tv_sec  = KEVENT_TIMEOUT_MILISEC / 1000;
  _timeOut.tv_nsec = KEVENT_TIMEOUT_MILISEC % 1000 * 1000 * 1000;
}

std::vector<Event*>& EventHandler::getRoutedEvents(int server_id) { return _routedEvents[server_id]; }

void EventHandler::_appendNewEventToChangeList(int ident, int filter, int flag, Event* event) {
  struct kevent kevent;
  EV_SET(&kevent, ident, filter, flag, 0, 0, event);
  _changeList.push_back(kevent);
}

void EventHandler::removeConnection(Event& event) {
  int ret = close(event.clientFd);
  if (event.type == CGI_RESPONSE_READABLE) {
    FileManager::registerTempFileFd(event.keventId);
    _appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DELETE, NULL);
  }
  Log::log().syscall(ret, LOG_LOCATION, "(SYSCALL) close done", "(SYSCALL) close error", INFILE);
  Log::log()("closed fd", event.clientFd);
  Log::log().mark(ret != 0);
  _eventSet.erase(&event);
  delete &event;
}

void EventHandler::_addConnection(int listen_fd) {
  sockaddr_in addr;
  socklen_t   alen;
  int         client_fd = accept(listen_fd, (struct sockaddr*)&addr, &alen);  // addr 버리나?

  Log::log().syscall(client_fd, LOG_LOCATION, "(SYSCALL) connection accepted", "(SYSCALL) connection not accepted",
                     INFILE);
  Log::log()("Server Socket fd", listen_fd, INFILE);
  Log::log()("Client Socket fd", client_fd, INFILE);
  Log::log().mark(client_fd == -1);

  fcntl(client_fd, F_SETFL, O_NONBLOCK);
  Event* event = new Event(HTTP_REQUEST_READABLE, client_fd);

  _appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD, event);
  _appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD, event);
  _appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_DISABLE, event);  // 따로 해야 제대로 적용됨.

  _eventSet.insert(event);
}

void EventHandler::_checkConnectionTimeout(const timeval& base_time) {
  std::vector<Event*> v;

  for (std::set<Event*>::iterator it = _eventSet.begin(); it != _eventSet.end(); ++it) {
    Event& event = **it;
    int    interval;
    interval =
        (base_time.tv_sec - event.timestamp.tv_sec) * 1000 + (base_time.tv_usec - event.timestamp.tv_usec) / 1000;
    if (interval >= CONNECTION_TIMEOUT_MILISEC && event.type == HTTP_REQUEST_READABLE) {
      v.push_back(&event);
    }
  }
  for (std::vector<Event*>::iterator it = v.begin(); it != v.end(); ++it) {
    Event& event = **it;
    Log::log()(LOG_LOCATION, "CLOSE BECAUSE TIME OUT", INFILE);
    removeConnection(event);
  }
}

void EventHandler::_updateEventsTimestamp(int num_kevents) {
  for (int i = 0; i < num_kevents; ++i) {
    Event& event = *(Event*)_keventList[i].udata;
    gettimeofday(&event.timestamp, NULL);
  }
}

void EventHandler::_routeEvent(Event& event) {
  switch (event.type) {
    case CONNECTION_REQUEST:
      _checkConnectionTimeout(event.timestamp);
      _addConnection(event.keventId);
      gettimeofday(&event.timestamp, NULL);
      break;

    case HTTP_REQUEST_READABLE:
      event.httpRequest.storeChunk(event.clientFd);
      gettimeofday(&event.timestamp, NULL);
      if (event.httpRequest.isConnectionClosed()) {
        removeConnection(event);
      } else if (event.httpRequest.isParsingEnd()) {
        event.serverId = _router.findServerId(event.httpRequest);
        _routedEvents[event.serverId].push_back(&event);
        Log::log()(LOG_LOCATION, "(event routed) Http Request Readable", INFILE);
      }
      break;

    case HTTP_RESPONSE_WRITABLE:
      _routedEvents[event.serverId].push_back(&event);
      gettimeofday(&event.timestamp, NULL);
      Log::log()(LOG_LOCATION, "(event routed) Http Response Writable", INFILE);
      break;

    case CGI_RESPONSE_READABLE:
      event.cgiResponse.readCgiResult(event.keventId, event.pid, event.baseClock);
      gettimeofday(&event.timestamp, NULL);
      if (event.cgiResponse.isReadError()) {
        removeConnection(event);
      } else if (event.cgiResponse.isParsingEnd()) {
        _routedEvents[event.serverId].push_back(&event);
        Log::log()(LOG_LOCATION, "(event routed) Cgi Reponse Readable", INFILE);
      }
      break;

    default:
      break;
  }
}

void EventHandler::routeEvents() {
  int num_kevents = kevent(_kQueue, _changeList.data(), _changeList.size(), _keventList, MAX_EVENTS, &_timeOut);
  _changeList.clear();
  _routedEvents.clear();
  Log::log().syscall(num_kevents, LOG_LOCATION, "", "(SYSCALL) kevent error", INFILE);
  Log::log().mark(num_kevents == -1);

  if (num_kevents == 0) {
    Log::log()("Wating event...", CONSOLE);
    timeval current;
    gettimeofday(&current, NULL);
    _checkConnectionTimeout(current);
  }

  FileManager::clearTempFileFd();
  _updateEventsTimestamp(num_kevents);

  for (int i = 0; i < num_kevents; ++i) {
    Event& event = *(Event*)_keventList[i].udata;
    int    flags = _keventList[i].flags;

    if (flags & EV_EOF) {
      Log::log()(LOG_LOCATION, "ev_eof", ALL);
      removeConnection(event);
      continue;
    }
    _routeEvent(event);
  }
}
