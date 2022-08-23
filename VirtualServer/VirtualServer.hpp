#ifndef MYLITTLEWEBSERV_VIRTUALSERVER_HPP
#define MYLITTLEWEBSERV_VIRTUALSERVER_HPP

#include "Config.hpp"
#include "EventHandler.hpp"
#include "HttpResponse.hpp"

class VirtualServer {
 private:
  int           _serverId;
  ServerInfo&   _serverInfo;
  EventHandler& _eventHandler;

  void        _callCgi(Event& event);
  void        _sendResponse(int fd, HttpResponse& response);
  std::string _findLocation(HttpRequest& httpRequest);

 public:
  VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler);
  void        start();
  ServerInfo& getServerInfo() const;
};

#endif
