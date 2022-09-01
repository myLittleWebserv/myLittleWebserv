#ifndef MYLITTLEWEBSERV_VIRTUALSERVER_HPP
#define MYLITTLEWEBSERV_VIRTUALSERVER_HPP

#include "Config.hpp"
#include "EventHandler.hpp"

class HttpResponse;

class VirtualServer {
 private:
  int           _serverId;
  ServerInfo&   _serverInfo;
  EventHandler& _eventHandler;

  void          _callCgi(Event& event);
  void          _sendResponse(int fd, HttpResponse& response);
  LocationInfo& _findLocationInfo(HttpRequest& httpRequest);
  std::string   _pidToString(int pid);
  void          _setEnv(Event& event, const std::string& cgi_path) const;

 public:
  VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler);
  void        start();
  ServerInfo& getServerInfo() const;
};

#endif
