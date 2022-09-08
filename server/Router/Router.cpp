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
  try {
    _serverSocketsInit();
  } catch (const std::exception& e) {
    Log::log()(LOG_LOCATION, e.what(), ALL);
    exit(1);
  }

  while (1) {
    _eventHandler.routeEvents();
    for (std::vector<VirtualServer>::size_type i = 0; i < _virtualServers.size(); ++i) {
      _virtualServers[i].start();
    }
  }
}

void Router::_serverSocketsInit() {
  for (std::set<int>::iterator port = _config.getPorts().begin(); port != _config.getPorts().end(); ++port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
      Log::log().syscall(server_socket, LOG_LOCATION, "", "(SYSCALL) socket error", ALL);
      throw ServerSocketInitException();
    }

    int ret;
    int sock_opt = 1;

    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));
    if (ret < 0) {
      Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) setsockopt error", ALL);
      throw ServerSocketInitException();
    }

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
    if (ret < 0) {
      Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) bind error", ALL);
      Log::log()(ret == -1, "port", *port);
      Log::log()(ret == -1, "addr", inet_ntoa(server_addr.sin_addr));
      Log::log()(ret == -1, "socket fd", server_socket);
      throw ServerSocketInitException();
    }

    ret = listen(server_socket, BACKLOG);
    if (ret < 0) {
      Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) listen error", ALL);
      throw ServerSocketInitException();
    }

    ret = fcntl(server_socket, F_SETFL, O_NONBLOCK);
    if (ret < 0) {
      Log::log().syscall(ret, LOG_LOCATION, "", "(SYSCALL) fcntl error", ALL);
      throw ServerSocketInitException();
    }

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
  Log::log()(LOG_LOCATION, "(Not Found) ServerId ", INFILE);
  Log::log()(true, "HttpRequest.hostPort", request.hostPort(), INFILE);
  Log::log()(true, "HttpRequest.hostName", request.hostName(), INFILE);
  return 0;
}
