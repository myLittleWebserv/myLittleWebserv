#include "Router.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <cstring>

#include "Log.hpp"
#include "syscall.hpp"

#define BACKLOG 1024

Router::Router(const std::string& confFile) : _config(confFile), _virtualServers(), _eventHandler(*this) {
  for (std::vector<VirtualServer>::size_type id = 0; id < _config.getServerInfos().size(); ++id) {
    _virtualServers.push_back(VirtualServer(id, _config.getServerInfos()[id], _eventHandler));
  }
}

void Router::start() {
  try {
    _serverSocketsInit();
  } catch (const std::exception& e) {
    Log::log()(LOG_LOCATION, e.what(), ALL);
    std::exit(1);
  }

  while (1) {
    _eventHandler.routeEvents();
    for (std::vector<VirtualServer>::size_type i = 0; i < _virtualServers.size(); ++i) {
      _virtualServers[i].start();
    }
  }
}

void Router::end() {
  for (std::vector<int>::iterator socket = _serverSockets.begin(); socket != _serverSockets.end(); ++socket) {
    close(*socket);
  }
}

void Router::_serverSocketsInit() {
  for (std::set<int>::iterator port = _config.getPorts().begin(); port != _config.getPorts().end(); ++port) {
    int server_socket = ft::syscall::socket(AF_INET, SOCK_STREAM, 0);
    int sock_opt      = 1;
    ft::syscall::setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

    sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(*port),
        .sin_addr =
            {
                .s_addr = htonl(INADDR_ANY),
            },
        .sin_zero =
            {
                0,
            },
    };

    ft::syscall::bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    ft::syscall::listen(server_socket, BACKLOG);
    ft::syscall::fcntl(server_socket, F_SETFL, O_NONBLOCK, ServerSocketInitException());
    _serverSockets.push_back(server_socket);
    Event* event = new Event(CONNECTION_REQUEST, server_socket);
    _eventHandler.addReadEvent(server_socket, event);
  }
  Log::log()(LOG_LOCATION, "(SUCCESS) Server Initialization", INFILE);
}

int Router::findServerId(HttpRequest& request) const {
  for (std::vector<VirtualServer>::size_type i = 0; i < _virtualServers.size(); ++i) {
    if (_virtualServers[i].getServerInfo().hostPort == request.hostPort() &&
        _virtualServers[i].getServerInfo().serverName == request.hostName()) {
      return i;
    }
  }
  for (std::vector<VirtualServer>::size_type i = 0; i < _virtualServers.size();
       ++i) {  // default server if no matching host:port
    if (_virtualServers[i].getServerInfo().hostPort == request.hostPort()) {
      return i;
    }
  }
  return 0;
}
