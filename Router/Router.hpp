#ifndef MYLITTLEWEBSERV_ROUTER_HPP
#define MYLITTLEWEBSERV_ROUTER_HPP

#include <vector>

#include "Config.hpp"
#include "EventHandler.hpp"
#include "VirtualServer.hpp"

class Router {
 private:
  Config                     _config;
  std::vector<VirtualServer> _virtualServers;
  EventHandler               _eventHandler;

  void serverSocketsInit();

 public:
  Router(const Config& config);
  void start();
  int  findServerId(HttpRequest& request);
};

#endif
