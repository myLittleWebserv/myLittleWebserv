#ifndef MYLITTLEWEBSERV_ROUTER_H
#define MYLITTLEWEBSERV_ROUTER_H

#include "Config.h"
#include "VirtualServer.h"
#include "EventHandler.h"
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

