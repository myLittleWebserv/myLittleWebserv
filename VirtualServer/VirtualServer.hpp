#ifndef MYLITTLEWEBSERV_VIRTUALSERVER_HPP
#define MYLITTLEWEBSERV_VIRTUALSERVER_HPP

#include "Config.hpp"
#include "EventHandler.hpp"
#include "HttpResponse.hpp"

class VirtualServer {
 private:
  int        _serverId;
  ServerInfo _serverInfo;

  void callCgi(Event* event);
  void sendResponse(int fd, HttpResponse& response);

 public:
  VirtualServer(int id, ServerInfo info);
  void start(EventHandler& eventHandler);
};

#endif
