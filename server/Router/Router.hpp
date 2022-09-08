#ifndef MYLITTLEWEBSERV_ROUTER_HPP
#define MYLITTLEWEBSERV_ROUTER_HPP

#include <exception>
#include <vector>

#include "Config.hpp"
#include "EventHandler.hpp"
#include "VirtualServer.hpp"

class Router {
 private:
  Config                     _config;
  std::vector<VirtualServer> _virtualServers;
  EventHandler               _eventHandler;

  void _serverSocketsInit();

 public:
  class ServerSocketInitException : public std::exception {
   public:
    virtual const char* what() const throw() { return "(ERROR) failed to init server sockets!"; }
  };
  class ServerSystemCallException : public std::exception {
   public:
    virtual const char* what() const throw() { return "(ERROR) failed to system-call!"; }
  };

 public:
  Router(const std::string& confFile);
  void start();
  int  findServerId(HttpRequest& request) const;
};

#endif
