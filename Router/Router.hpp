#ifndef MYLITTLEWEBSERV_ROUTER_HPP
#define MYLITTLEWEBSERV_ROUTER_HPP

#include <vector>

#include "Config.hpp"
#include "EventHandler.hpp"
#include "VirtualServer.hpp"

class Router {
 private:
  Config                     _config;
  std::vector<VirtualServer> _virtual_servers;
  EventHandler               _event_handler;

 public:
  Router(const Config& config);
  void start();
};

#endif
