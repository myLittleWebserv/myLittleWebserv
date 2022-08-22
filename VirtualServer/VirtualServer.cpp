#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

VirtualServer::VirtualServer(int id, ServerInfo info) : _serverId(id), _serverInfo(info) {}

void VirtualServer::start(EventHandler& eventHandler) {
  std::vector<Event*> event_list = eventHandler.getRoutedEvents(_serverId);
  for (int i = 0; i < event_list.size(); i++) {
    Event* event = event_list[i];
    switch (event->type) {
      case CONNECTION_REQUEST:
        eventHandler.addConnection(event->keventId);
        event->type = HTTP_REQUEST_READABLE;
        break;
      case HTTP_REQUEST_READABLE:
        if (event->httpRequest.isEnd()) {
          if (event->httpRequest.isCgi(_serverInfo.locations)) {
            callCgi(event);
            break;
          }
          event->type = HTTP_RESPONSE_WRITABLE;
          eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_DISABLE, event);
          eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD | EV_ENABLE, event);
        } else {
          eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_ENABLE, event);
        }
        break;
      case HTTP_RESPONSE_WRITABLE:
        sendResponse(event->keventId, HttpResponse(event->httpRequest));
        if (!event->httpRequest->isKeepAlive()) {
          eventHandler.removeConnection(event);
        }
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_DISABLE, event);
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD | EV_DISABLE, event);
        delete event;
        break;
      case CGI_RESPONSE_READABLE:
        sendResponse(event->keventId, HttpResponse(event->cgiResponse));
        if (!event->httpRequest->isKeepAlive()) {
          eventHandler.removeConnection(event);
        }
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD | EV_DISABLE, event);
        eventHandler.appendNewEventToChangeList(event->keventId, EVFILT_WRITE, EV_ADD | EV_DISABLE, event);
        delete event;
        break;
      default:
        break;
    }
  }
}

#define READEND 0
#define WRITEEND 1

void VirtualServer::callCgi(Event* event) {
  //  std::string cgi_path = getCgiPath(_serverInfo.locations);
  int to_pipe[2];  // 0 = readend, 1 = writeend
  int from_pipe[2];

  if (::pipe(to_pipe) == -1 || ::pipe(from_pipe) == -1) {
    throw "error";
  }
  int env = 0;
  env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  env += setenv("REQUEST_METHOD", "GET", 1);
  env += setenv("PATH_INFO", cgiPath.c_str(), 1);
  if (env != 0) {
    throw "error";
  }
  fcntl(to_pipe[WRITEEND], F_SETFL, O_NONBLOCK);
  close(to_pipe[READEND]);
  write(to_pipe[WRITEEND], event->httpRequest.getBody().c_str(), event->httpRequest.getBody().size());
  close(to_pipe[WRITEEND]);
  event->pid = fork();
  if (event->pid == -1) {
    throw "error";
  } else if (event->pid == 0) {  // child



  } else {  // parent
  }
}

void sendResponse(int fd, HttpResponse& response) {
  std::string response_str = response.getResponse();
  if (send(fd, response_str.c_str(), response_str.size(), 0) == -1) {
    throw "send() error!";
  }
}
