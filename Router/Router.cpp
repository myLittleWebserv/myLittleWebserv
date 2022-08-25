#include "Router.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <cstring>

#include "Log.hpp"

#define BACKLOG 1024

Router::Router(const std::string& confFile) : _config(confFile), _virtualServers(), _eventHandler(*this) {
  for (std::vector<VirtualServer>::size_type id = 0; id < _config.getServerInfos().size(); ++id) {
    _virtualServers.push_back(VirtualServer(id, _config.getServerInfos()[id], _eventHandler));
  }
}

void Router::start() {
  _serverSocketsInit();
  while (1) {
    _eventHandler.routeEvents();
    for (std::vector<VirtualServer>::size_type i = 0; i < _virtualServers.size(); ++i) {
      _virtualServers[i].start();
    }
  }
}

void Router::_serverSocketsInit() {
  for (std::vector<int>::iterator port = _config.getPorts().begin(); port != _config.getPorts().end(); ++port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    Log::log().syscall(server_socket, LOG_LOCATION, "", "(SYSCALL) socket error", ALL);
    Log::log().mark(server_socket == -1);

    int ret;
    int sock_opt = 1;

    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));
    Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) setsockopt error", ALL);
    Log::log().mark(ret == -1);

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

    ret = bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
    Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) bind error", ALL);
    Log::log()(ret == -1, "port", *port);
    Log::log()(ret == -1, "addr", inet_ntoa(server_addr.sin_addr));
    Log::log()(ret == -1, "socket fd", server_socket);
    Log::log().mark(ret == -1);

    ret = listen(server_socket, BACKLOG);
    Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) listen error", ALL);
    Log::log()(ret == -1, "socket fd", server_socket);
    Log::log().mark(ret == -1);

    ret = fcntl(server_socket, F_SETFL, O_NONBLOCK);
    Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) fcntl error", ALL);
    Log::log().mark(ret == -1);

    Event* event = new Event(CONNECTION_REQUEST, server_socket);
    _eventHandler.appendNewEventToChangeList(server_socket, EVFILT_READ, EV_ADD, event);
  }
  Log::log()(LOG_LOCATION, "(SUCCESS) Server Initialization", ALL);
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
  Log::log()(LOG_LOCATION, "(Not Found) ServerId ", ALL);
  Log::log()("HttpRequest.hostPort", request.hostPort());
  Log::log()("HttpRequest.hostName", request.hostName());
  throw std::string("invalid ServerId");
  return -1;
}
