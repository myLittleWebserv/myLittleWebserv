#include "Router.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <cstring>

#define BACKLOG 1024

Router::Router(const std::string& confFile) : _config(confFile), _virtualServers(), _eventHandler(*this) {
  for (int id = 0; id < _config.getServerInfos().size(); ++id) {
    _virtualServers.push_back(VirtualServer(id, _config.getServerInfos()[id], _eventHandler));
  }
}

void Router::start() {
  _serverSocketsInit();
  while (1) {
    _eventHandler.routeEvents();
    for (int i = 0; i < _virtualServers.size(); ++i) {
      _virtualServers[i].start();
    }
  }
}

void Router::_serverSocketsInit() {
  for (std::vector<int>::iterator port = _config.getPorts().begin(); port != _config.getPorts().end(); ++port) {
    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
      throw "socket() error!";
    }

    int sock_opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family         = AF_INET;
    server_addr.sin_addr.s_addr    = htonl(INADDR_ANY);
    server_addr.sin_port           = htons(*port);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
      throw "bind() error!";
    }

    if (listen(server_socket, BACKLOG) == -1) {
      throw "listen() error!";
    }
    fcntl(server_socket, F_SETFL, O_NONBLOCK);

    Event* event = new Event(CONNECTION_REQUEST, server_socket);
    _eventHandler.appendNewEventToChangeList(server_socket, EVFILT_READ, EV_ADD | EV_ENABLE, event);
  }
}

int Router::findServerId(HttpRequest& request) const {
  for (int i = 0; i < _virtualServers.size(); ++i) {
    if (_virtualServers[i].getServerInfo().hostPort == request.hostPort() &&
        _virtualServers[i].getServerInfo().serverName == request.hostName()) {
      return i;
    }
  }
  for (int i = 0; i < _virtualServers.size(); ++i) {  // default server if no matching host:port
    if (_virtualServers[i].getServerInfo().hostPort == request.hostPort()) {
      return i;
    }
  }
  return -1;
}
