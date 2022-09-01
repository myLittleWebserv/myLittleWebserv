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

  bool          _callCgi(Event& event);
  void          _sendResponse(int fd, HttpResponse& response);
  LocationInfo& _findLocationInfo(HttpRequest& httpRequest);
  std::string   _intToString(int integer);
  void          _setEnv(int request_method, const std::string& cgi_path) const;
  void          _setFd(int request, int response) const;
  void          _processSendingEnd(Event& event);
  void          _processHttpRequestReadable(Event& event, LocationInfo& location_info);
  void          _processCgiResponseReadable(Event& event, LocationInfo& location_info);

 public:
  VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler);
  void        start();
  ServerInfo& getServerInfo() const;
};

#endif
