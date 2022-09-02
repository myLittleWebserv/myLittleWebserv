#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (std::vector<Event*>::size_type i = 0; i < event_list.size(); i++) {
    Event&        event         = *event_list[i];
    LocationInfo& location_info = _findLocationInfo(event.httpRequest);

    switch (event.type) {
      case HTTP_REQUEST_READABLE:
        _processHttpRequestReadable(event, location_info);
        break;

      case CGI_RESPONSE_READABLE:
        _processCgiResponseReadable(event, location_info);
        break;

      case HTTP_RESPONSE_WRITABLE:
        Log::log().printHttpResponse(*event.httpResponse, ALL);
        _sendResponse(event.clientFd, *event.httpResponse);
        if (event.httpResponse->storage().empty()) {
          _processSendingEnd(event);
        }
        break;

      default:
        break;
    }
  }
}

void VirtualServer::_processSendingEnd(Event& event) {
  if (event.httpRequest.isKeepAlive()) {
    event.initialize();
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ENABLE, &event);
  } else {
    _eventHandler.removeConnection(event);
  }
  Log::log()(LOG_LOCATION, "(DONE) sending Http Response", ALL);
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  Log::log().printHttpRequest(event.httpRequest, ALL);
  if (event.httpRequest.isCgi(location_info.cgiExtension) && _callCgi(event)) {
    return;
  }
  event.httpResponse = new HttpResponse(event.httpRequest, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
  Log::log()(LOG_LOCATION, "(DONE) making Http Response", ALL);
}

void VirtualServer::_processCgiResponseReadable(Event& event, LocationInfo& location_info) {
  event.httpResponse = new HttpResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;
  if (close(event.keventId) == -1)
    throw "close error";
  event.keventId = event.clientFd;
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
  Log::log()(LOG_LOCATION, "(DONE) making Http Response", ALL);
}

LocationInfo& VirtualServer::_findLocationInfo(HttpRequest& httpRequest) {
  std::string                                   key;
  std::map<std::string, LocationInfo>::iterator found;

  key = httpRequest.uri();
  while (key != "/" && !key.empty()) {
    found = _serverInfo.locations.find(key);
    if (found != _serverInfo.locations.end()) {
      Log::log()(LOG_LOCATION, "LocationInfo found", ALL);
      Log::log()(true, "key", found->first, ALL);
      return found->second;
    }
    key = key.substr(0, key.rfind('/'));
  }
  Log::log()(LOG_LOCATION, "LocationInfo is default", ALL);
  return _serverInfo.locations["/"];
}

bool VirtualServer::_callCgi(Event& event) {
  std::string fd           = _intToString(event.clientFd);
  std::string req_filepath = "temp/request" + fd;
  std::string res_filepath = "temp/response" + fd;
  int         request      = open(req_filepath.c_str(), O_RDWR | O_CREAT, 0644);
  int         response     = open(res_filepath.c_str(), O_RDWR | O_CREAT, 0644);

  if (request == -1 || response == -1) {
    unlink(res_filepath.c_str());
    unlink(req_filepath.c_str());
    event.httpRequest.setServerError(true);
    return false;
  }

  event.pid = fork();
  if (event.pid == -1) {
    event.httpRequest.setServerError(true);
    return false;
  } else if (event.pid == 0) {  // child
    if (write(request, event.httpRequest.body().data(), event.httpRequest.body().size()) == -1)
      std::exit(EXIT_FAILURE);

    std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
    char*       argv[2]  = {0, 0};
    argv[0]              = strdup(cgi_path.c_str());

    _setEnv(event.httpRequest.method(), cgi_path);
    _setFd(request, response);

    execve(cgi_path.c_str(), argv, NULL);
    std::exit(EXIT_FAILURE);
  } else {  // parent
    close(request);
    event.keventId = response;
    event.type     = CGI_RESPONSE_READABLE;
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ADD, &event);
    return true;
  }
}

void VirtualServer::_setEnv(int request_method, const std::string& cgi_path) const {
  std::string method;
  switch (request_method) {
    case GET:
      method = "GET";
      break;
    case POST:
      method = "POST";
      break;
    case HEAD:
      method = "HEAD";
      break;
    default:
      std::exit(EXIT_FAILURE);
  }
  int env = 0;
  env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  env += setenv("REQUEST_METHOD", method.c_str(), 1);
  env += setenv("PATH_INFO", cgi_path.c_str(), 1);
  if (env != 0) {
    std::exit(EXIT_FAILURE);
  }
}

void VirtualServer::_setFd(int request, int response) const {
  if (dup2(request, STDIN_FILENO) == -1)
    std::exit(EXIT_FAILURE);
  if (dup2(response, STDOUT_FILENO) == -1)
    std::exit(EXIT_FAILURE);
  if (close(request) == -1)
    std::exit(EXIT_FAILURE);
  if (close(response) == -1)
    std::exit(EXIT_FAILURE);
}

std::string VirtualServer::_intToString(int integer) {
  std::stringstream ss;
  ss << integer;
  return ss.str();
}

void VirtualServer::_sendResponse(int fd, HttpResponse& response) {
  send(fd, response.storage().data(), response.storage().size(), 0);
  Log::log()(LOG_LOCATION, "(SYSCALL) send HttpResponse to client ", ALL);
  //  storage 사용 예정
  //  if (!response.headerSent()) {
  //    send(fd, response.headerToString().c_str(), response.headerToString().size(), 0);
  //    response.toggleHeaderSent();
  //    Log::log()(LOG_LOCATION, "(SYSCALL) send HttpResponse header to client ", ALL);
  //  }
  //  if ((int)response.body().size() == response.sentLength()) {
  //    return;
  //  }
  //  int len;
  //  if ((len = send(fd, response.body().data() + response.sentLength(), response.body().size() -
  //  response.sentLength(),
  //                  0)) == -1) {
  //    throw "send() error!";
  //  }
  //  response.addSentLength(len);
}

ServerInfo& VirtualServer::getServerInfo() const { return _serverInfo; }
