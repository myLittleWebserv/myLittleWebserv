#ifndef MYLITTLEWEBSERV_ROUTER_HPP
#define MYLITTLEWEBSERV_ROUTER_HPP

#include <vector>
#include <exception>

#include "Config.hpp"
#include "EventHandler.hpp"
#include "VirtualServer.hpp"

class Router {
 private:
  Config                     _config;
  std::vector<VirtualServer> _virtualServers;
  EventHandler               _eventHandler;

  void _serverSocketsInit();
  class ServerSocketInitException : public std::exception {
   public:
    virtual const char* what() const throw() { return "(ERROR) failed to init server sockets!"; }
  };

 public:
  Router(const std::string& confFile);
  void start();
  int  findServerId(HttpRequest& request) const;
};

#endif
