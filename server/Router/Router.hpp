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
  std::vector<int>           _serverSockets;

  void _serverSocketsInit();

 public:
  class ServerSocketInitException : public std::exception {
   public:
    virtual const char* what() const throw() { return "(ERROR) failed to init server sockets!"; }
  };
  class ServerSystemCallException : public std::exception {
   private:
    const char* _msg;

   public:
    ServerSystemCallException(const char* msg) throw() : _msg(msg) {}
    virtual const char* what() const throw() { return _msg; }
  };

 public:
  Router(const std::string& confFile);
  void start();
  void end();
  int  findServerId(HttpRequest& request) const;
};

#endif
