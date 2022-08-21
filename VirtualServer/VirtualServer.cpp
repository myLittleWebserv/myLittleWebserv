#include "VirtualServer.hpp"

#include <sys/socket.h>

VirtualServer::VirtualServer(int id, ServerInfo info) : _serverId(id), _serverInfo(info) {}

void VirtualServer::start(EventHandler& eventHandler) {
  std::vector<Event *> event_list = eventHandler.getRoutedEvents(_serverId);
  for (int i = 0; i < event_list.size(); i++) {
    Event* event = event_list[i];
    switch (event->type) {
      case CONNECTION_REQUEST:
        eventHandler.addConnection(event->keventId);
        break;
      case HTTP_REQUEST_READABLE:
          eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_ENABLE, event);
        break;
      case HTTP_RESPONSE_WRITABLE:
        if (event->httpRequest->isCgi()) {
          callCgi(event);
          break ;
        }
        sendResponse(event->keventId, HttpResponse(event->httpRequest));
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_DISABLE, event);
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD | EV_ENABLE, event);
        if (!event->httpRequest->isKeepAlive()) {
          eventHandler.removeConnection(event);
        }
        delete event;
        break;
      case CGI_RESPONSE_WRITABLE:
        sendResponse(event->keventId, HttpResponse(event->cgiResponse));
        if (!event->httpRequest->isKeepAlive()) {
          eventHandler.removeConnection(event);
        }
        delete event;
        break;
      default:
        break;
    }
  }
}

void VirtualServer::callCgi(Event* event) {

}

void sendResponse(int fd, HttpResponse& response) {
  std::string response_str = response.getResponse();
  if (send(fd, response_str.c_str(), response_str.size(), 0) == -1) {
    throw "send() error!";
  }
}

