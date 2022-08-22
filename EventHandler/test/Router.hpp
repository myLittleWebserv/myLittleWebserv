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
  Router(const Config& config) : _config(config), _event_handler(*this) {}
  void start();
  int  findServerId(const HttpRequest& httpRequest) const {
     if (httpRequest._hostPort == 7777) {
       return 1;
    } else {
       return 2;
    }
  }
};

#endif
