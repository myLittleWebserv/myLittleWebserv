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
    LocationInfo& location_info = _findLocationInfo(event.httpRequest);  // ?

    switch (event.type) {
      case HTTP_REQUEST_READABLE:
        Log::log().printHttpRequest(event.httpRequest, INFILE);
        _processHttpRequestReadable(event, location_info);
        Log::log()(true, "HTTP_REQUEST_READABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
        break;

      case CGI_REQUEST_WRITABLE:
        _sendCgiRequest(event.keventId, event.httpRequest);
        event.type = CGI;
        if (event.httpRequest.body().empty()) {
          _finishCgiRequest(event);
          Log::log()(true, "CGI_REQUEST_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
        }
        break;

      case CGI_RESPONSE_READABLE:
        _cgiResponseToHttpResponse(event, location_info);
        Log::log()(true, "CGI_RESPONSE_READABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
        break;

      case HTTP_RESPONSE_WRITABLE:
        Log::log().printHttpResponse(*event.httpResponse, INFILE);
        _sendResponse(event.clientFd, *event.httpResponse);
        if (event.httpResponse->storage().empty()) {
          _finishResponse(event);
          Log::log()(true, "HTTP_RESPONSE_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC,
                     ALL);
        }
        break;

      default:
        break;
    }
  }
}

void VirtualServer::_finishCgiRequest(Event& event) {
  event.type = CGI;
  // _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ENABLE, NULL);
  // _eventHandler.appendNewEventToChangeList(event.pipeFd, EVFILT_READ, EV_ADD, &event);
  close(event.keventId);  // ?
  event.keventId = event.pipeFd;
}

void VirtualServer::_sendCgiRequest(int to_fd, HttpRequest& request) {
  int write_size = write(to_fd, request.body().currentReadPos(), request.body().remains());
  if (write_size == -1) {
    Log::log()(true, "errno", strerror(errno), INFILE);
    return;
  }
  Log::log()(LOG_LOCATION, "(SYSCALL) send CgiRequest to CGI ", INFILE);
  Log::log()(true, "send fd", to_fd, INFILE);
  Log::log()(true, "sent_size", write_size, INFILE);
  request.body().moveReadPos(write_size);
  Log::log()(true, "remains", request.body().remains(), INFILE);
}

void VirtualServer::_finishResponse(Event& event) {
  if (event.httpRequest.isKeepAlive() || !event.httpRequest.storage().empty()) {
    event.initialize();
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_ENABLE, &event);
  } else {
    _eventHandler.removeConnection(event);
  }
  Log::log()(LOG_LOCATION, "(DONE) sending Http Response", INFILE);
  Log::log().printStatus();
  Log::log().increaseProcessedConnection();
  Log::log().mark();
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  if (event.httpRequest.isCgi(location_info.cgiExtension) && _callCgi(event)) {
    event.type = CGI;
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ADD, &event);
    _eventHandler.appendNewEventToChangeList(event.pipeFd, EVFILT_READ, EV_ADD, &event);
  } else {
    event.httpResponse = new HttpResponse(event.httpRequest, location_info);
    event.type         = HTTP_RESPONSE_WRITABLE;
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
    Log::log()(LOG_LOCATION, "(DONE) making Http Response using HTTP REQUEST", INFILE);
  }
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  FileManager file_manager;
  event.httpResponse = new HttpResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;

  // file_manager.removeFile(event.clientFd);
  file_manager.registerTempFileFd(event.keventId);
  _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DELETE, NULL);
  // close(event.keventId);  // ?
  _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ENABLE, &event);
  Log::log()(LOG_LOCATION, "(DONE) making Http Response using CGI RESPONSE", INFILE);
}

LocationInfo& VirtualServer::_findLocationInfo(HttpRequest& httpRequest) {
  std::string                                   key;
  std::map<std::string, LocationInfo>::iterator found;

  key = httpRequest.uri();
  while (key != "/" && !key.empty()) {
    found = _serverInfo.locations.find(key);
    if (found != _serverInfo.locations.end()) {
      Log::log()(LOG_LOCATION, "LocationInfo found", INFILE);
      Log::log()(true, "key", found->first, INFILE);
      return found->second;
    }
    key = key.substr(0, key.rfind('/'));
  }
  Log::log()(LOG_LOCATION, "LocationInfo is default", INFILE);
  return _serverInfo.locations["/"];
}

bool VirtualServer::_callCgi(Event& event) {
  int w_pipe[2];
  int r_pipe[2];
  int ret = 0;

  ret += pipe(w_pipe);
  ret += pipe(r_pipe);
  Log::log().syscall(ret < 0, LOG_LOCATION, "", "(SYSCALL) pipe failed", INFILE);
  Log::log().mark(ret == -1);

  event.pid = fork();
  if (event.pid == -1) {
    event.httpRequest.setServerError(true);
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", INFILE);
    return false;
  } else if (event.pid == 0) {
    close(w_pipe[1]);
    close(r_pipe[0]);
    _execveCgi(event, w_pipe[0], r_pipe[1]);  // child
  } else {                                    // parent
    close(w_pipe[0]);
    close(r_pipe[1]);
    int ret = fcntl(w_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(r_pipe[0], F_SETFL, O_NONBLOCK);
    Log::log()(true, "ret", ret, INFILE);
    Log::log()(true, "errno", strerror(errno), INFILE);

    Log::log()(LOG_LOCATION, "PIPE FD", INFILE);
    Log::log()(true, "PIPE write", w_pipe[1], INFILE);
    Log::log()(true, "PIPE read ", r_pipe[0], INFILE);

    event.keventId = w_pipe[1];
    event.pipeFd   = r_pipe[0];
  }
  Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", INFILE);
  event.cgiResponse.setInfo(event.httpRequest);
  return true;
}

void VirtualServer::_execveCgi(Event& event, int read_end, int write_end) {
  std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
  char*       argv[2]  = {0, 0};
  char*       envp[5]  = {0, 0, 0, 0, 0};
  argv[0]              = strdup(cgi_path.c_str());

  _setEnv(event.httpRequest, cgi_path, envp);
  _setFd(read_end, write_end);

  execve(cgi_path.c_str(), argv, envp);
  Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after execve", INFILE);
  std::exit(EXIT_FAILURE);
}

void VirtualServer::_setEnv(const HttpRequest& http_request, const std::string& cgi_path, char** envp) const {
  std::string       server_protocol("SERVER_PROTOCOL=");
  std::string       request_method("REQUEST_METHOD=");
  std::string       path_info("PATH_INFO=");
  std::stringstream secret_header_for_test;

  switch (http_request.method()) {
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

  server_protocol += http_request.httpVersion();
  path_info += cgi_path;
  Log::log()(true, "server_protocol", server_protocol, INFILE);
  Log::log()(true, "request_method", request_method, INFILE);
  Log::log()(true, "path_info", path_info, INFILE);
  Log::log()(true, "secret_header_for_test", secret_header_for_test.str(), INFILE);
  envp[0] = strdup(server_protocol.c_str());
  envp[1] = strdup(request_method.c_str());
  envp[2] = strdup(path_info.c_str());
  if (http_request.secretHeaderForTest() > 0) {
    secret_header_for_test << "HTTP_X_SECRET_HEADER_FOR_TEST=";
    secret_header_for_test << http_request.secretHeaderForTest();
    envp[3] = strdup(secret_header_for_test.str().c_str());
  }

  if (envp[0] == NULL || envp[1] == NULL || envp[2] == NULL) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after setenv", INFILE);
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
  int sent_size = send(fd, response.storage().currentReadPos(), response.storage().remains(), 0);
  Log::log()(LOG_LOCATION, "(SYSCALL) send HttpResponse to client ", INFILE);
  Log::log()(true, "send fd", fd, INFILE);
  Log::log()(true, "sent_size", sent_size, INFILE);
  if (sent_size == -1) {
    Log::log()(true, "errno", strerror(errno), INFILE);
    return;
  }
  response.storage().moveReadPos(sent_size);
  Log::log()(true, "remains", response.storage().remains(), INFILE);
}

ServerInfo& VirtualServer::getServerInfo() const { return _serverInfo; }
