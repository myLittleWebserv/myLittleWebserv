#ifndef MYLITTLEWEBSERV_ROUTER_HPP
#define MYLITTLEWEBSERV_ROUTER_HPP

#include "Config.hpp"
#include "VirtualServer.hpp"
#include "EventHandler.hpp"
#include <vector>

class Router {
  private:
    Config _config;
    std::vector<VirtualServer> _virtual_servers;
    EventHandler _event_handler;

  public:
    Router(const Config& config);
    void start();
};

#endif

