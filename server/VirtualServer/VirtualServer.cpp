#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "FileManager.hpp"

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (std::vector<Event*>::size_type i = 0; i < event_list.size(); i++) {
    Event&        event         = *event_list[i];
    LocationInfo& location_info = _findLocationInfo(event.httpRequest);

    switch (event.type) {
      case HTTP_REQUEST_READABLE:
        Log::log().printHttpRequest(event.httpRequest, ALL);
        _processHttpRequestReadable(event, location_info);
        break;

      case CGI_RESPONSE_READABLE:
        _cgiResponseToHttpResponse(event, location_info);
        break;

      case HTTP_RESPONSE_WRITABLE:
        Log::log().printHttpResponse(*event.httpResponse, ALL);
        _sendResponse(event.clientFd, *event.httpResponse);
        if (event.httpResponse->storage().empty()) {
          _finishResponse(event);
        }
        break;

      default:
        break;
    }
  }
}

void VirtualServer::_finishResponse(Event& event) {
  if (event.httpRequest.isKeepAlive()) {
    event.initialize();
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_ENABLE, &event);
  } else {
    _eventHandler.removeConnection(event);
  }
  Log::log()(LOG_LOCATION, "(DONE) sending Http Response", ALL);
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  if (event.httpRequest.isCgi(location_info.cgiExtension) && _callCgi(event)) {
    return;
  }
  event.httpResponse = new HttpResponse(event.httpRequest, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
  Log::log()(LOG_LOCATION, "(DONE) making Http Response using HTTP REQUEST", ALL);
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  FileManager file_manager;
  event.httpResponse = new HttpResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;

  file_manager.removeFile(event.clientFd);
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DELETE, NULL);
  _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ENABLE, &event);
  // event.keventId = event.clientFd; ?
  Log::log()(LOG_LOCATION, "(DONE) making Http Response using CGI RESPONSE", ALL);
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
  std::string res_filepath = "temp/response" + fd;

  event.pid = fork();
  if (event.pid == -1) {
    event.httpRequest.setServerError(true);
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", ALL);
    return false;
  } else if (event.pid == 0) {
    _execveCgi(event);  // child
  } else {              // parent
    FileManager file_manager;
    int         cgi_response = file_manager.openFile(res_filepath.c_str(), O_RDONLY | O_CREAT, 0644);
    if (cgi_response == -1) {
      event.httpRequest.setServerError(true);
      Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", ALL);
      return false;
    }
    fcntl(cgi_response, F_SETFL, O_NONBLOCK);
    event.keventId = cgi_response;
    event.type     = CGI_RESPONSE_READABLE;
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ADD, &event);
  }
  Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", ALL);
  event.cgiResponse.setInfo(event.httpRequest);
  return true;
}

void VirtualServer::_execveCgi(Event& event) {
  std::string fd           = _intToString(event.clientFd);
  std::string req_filepath = "temp/request" + fd;
  std::string res_filepath = "temp/response" + fd;
  int         cgi_request  = open(req_filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  int         cgi_response = open(res_filepath.c_str(), O_WRONLY | O_CREAT, 0644);

  if (cgi_request == -1 || cgi_response == -1) {
    unlink(res_filepath.c_str());
    unlink(req_filepath.c_str());
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", ALL);
    std::exit(EXIT_FAILURE);
  }

  if (write(cgi_request, event.httpRequest.body().data(), event.httpRequest.body().size()) == -1) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after write", ALL);
    std::exit(EXIT_FAILURE);
  }

  // int ret = lseek(cgi_request, 0, SEEK_SET);
  cgi_request = open(req_filepath.c_str(), O_RDONLY, 0644);
  if (cgi_request == -1) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", ALL);
    std::exit(EXIT_FAILURE);
  }

  std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
  char*       argv[2]  = {0, 0};
  char*       envp[4]  = {0, 0, 0, 0};
  argv[0]              = strdup(cgi_path.c_str());

  _setEnv(event.httpRequest.method(), cgi_path, envp);
  _setFd(cgi_request, cgi_response);

  execve(cgi_path.c_str(), argv, envp);
  Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after execve", ALL);
  std::exit(EXIT_FAILURE);
}

void VirtualServer::_setEnv(int http_method, const std::string& cgi_path, char** envp) const {
  std::string request_method("REQUEST_METHOD=");
  std::string path_info("PATH_INFO=");

  switch (http_method) {
    case GET:
      request_method += "GET";
      break;
    case POST:
      request_method += "POST";
      break;
    case HEAD:
      request_method += "HEAD";
      break;
    default:
      std::exit(EXIT_FAILURE);
  }

  path_info += cgi_path;
  envp[0] = strdup("SERVER_PROTOCOL=HTTP/1.1");
  envp[1] = strdup(request_method.c_str());
  envp[2] = strdup(path_info.c_str());

  if (envp[0] == NULL || envp[1] == NULL || envp[2] == NULL) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after setenv", ALL);
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
  int send_size = send(fd, response.storage().data(), response.storage().size(), 0);
  Log::log()(LOG_LOCATION, "(SYSCALL) send HttpResponse to client ", ALL);
  Log::log()(true, "send fd", fd, ALL);
  Log::log()(true, "send_size", send_size, ALL);
  Log::log()(true, "errno", strerror(errno), ALL);

  response.storage().movePos(send_size);
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
