#include "VirtualServer.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstring>

#include "FileManager.hpp"

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (std::vector<Event*>::size_type i = 0; i < event_list.size(); i++) {
    Event& event = *event_list[i];
    _processEvent(event);
  }
}

ServerInfo& VirtualServer::getServerInfo() const { return _serverInfo; }

void VirtualServer::_processEvent(Event& event) {
  LocationInfo& location_info = _findLocationInfo(event.httpRequest);
  switch (event.type) {
    case HTTP_REQUEST_READABLE:
      Log::log().printHttpRequest(event.httpRequest, INFILE);
      _processHttpRequestReadable(event, location_info);
      Log::log()(true, "HTTP_REQUEST_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
      break;

    case CGI_RESPONSE_READABLE:
      _cgiResponseToHttpResponse(event, location_info);
      Log::log()(true, "CGI_RESPONSE_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
      break;

    case HTTP_RESPONSE_WRITABLE:
      Log::log().printHttpResponse(*event.httpResponse, INFILE);
      event.httpResponse->sendResponse(event.toSendFd);
      if (event.httpResponse->isConnectionClosed()) {
        _eventHandler.removeConnection(event);
      } else if (event.httpResponse->isSendingEnd()) {
        Log::log()(true, "HTTP_RESPONSE_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
        _finishResponse(event);
      }
      break;

    default:
      break;
  }
}

void VirtualServer::_finishResponse(Event& event) {
  if (event.keventId != event.toSendFd) {
    FileManager::registerTempFileFd(event.fileFd);
    FileManager::removeFile(event.toSendFd);
  }

  if (event.httpRequest.isKeepAlive() || !event.httpRequest.storage().empty()) {
    event.initialize();
    _eventHandler.disableWriteEvent(event.toSendFd, &event);
    _eventHandler.enableReadEvent(event.toSendFd, &event);
  } else {
    _eventHandler.removeConnection(event);
  }
  Log::log()(LOG_LOCATION, "(DONE) sending Http Response", INFILE);
  Log::log().increaseProcessedConnection();
  Log::log().printStatus();
  Log::log().mark();
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  if (event.httpRequest.isCgi(location_info.cgiExtension) && _callCgi(event)) {
    event.type = CGI_RESPONSE_READABLE;
    _eventHandler.disableReadEvent(event.toSendFd, &event);
    _eventHandler.addReadEvent(event.fileFd, &event);
  } else {
    event.httpResponse = new HttpResponse(event.httpRequest, location_info);
    event.type         = HTTP_RESPONSE_WRITABLE;
    _eventHandler.disableReadEvent(event.toSendFd, &event);
    _eventHandler.enableWriteEvent(event.toSendFd, &event);
    Log::log()(LOG_LOCATION, "(DONE) making Http Response using HTTP REQUEST", INFILE);
  }
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  event.httpResponse = new HttpResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;

  Log::log()(true, "CGI RES to HTTPRES     DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
  _eventHandler.deleteReadEvent(event.keventId, NULL);
  _eventHandler.enableWriteEvent(event.toSendFd, &event);
  Log::log()(LOG_LOCATION, "(DONE) making Http Response using CGI RESPONSE", INFILE);
}

LocationInfo& VirtualServer::_findLocationInfo(HttpRequest& httpRequest) {
  std::string                                   key;
  std::map<std::string, LocationInfo>::iterator found;

  key = httpRequest.uri();
  while (key != "/" && !key.empty()) {
    found = _serverInfo.locations.find(key);
    if (found != _serverInfo.locations.end()) {
      Log::log()(true, "LocationInfo found: key", found->first, INFILE);
      return found->second;
    }
    key = key.substr(0, key.rfind('/'));
  }
  Log::log()(true, "LocationInfo is default", INFILE);
  return _serverInfo.locations["/"];
}

bool VirtualServer::_callCgi(Event& event) {
  std::string fd           = _intToString(event.toSendFd);
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;

  event.pid = fork();
  if (event.pid == -1) {
    event.httpRequest.setServerError(true);
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", INFILE);
    return false;
  } else if (event.pid == 0) {
    _execveCgi(event);  // child
  } else {              // parent
    int cgi_response = open(res_filepath.c_str(), O_RDONLY | O_CREAT, 0644);
    if (cgi_response == -1) {
      event.httpRequest.setServerError(true);
      Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", INFILE);
      return false;
    }
    fcntl(cgi_response, F_SETFL, O_NONBLOCK);
    event.fileFd   = cgi_response;
    event.keventId = cgi_response;
    event.cgiResponse.setInfo(event.httpRequest);
    event.cgiResponse.getLine().setFd(cgi_response);
  }
  Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", INFILE);
  return true;
}

void VirtualServer::_execveCgi(Event& event) {
  event.baseClock          = clock();
  std::string fd           = _intToString(event.toSendFd);
  std::string req_filepath = TEMP_REQUEST_PREFIX + fd;
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;
  int         cgi_request  = open(req_filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
  int         cgi_response = open(res_filepath.c_str(), O_WRONLY | O_CREAT, 0644);

  if (cgi_request == -1 || cgi_response == -1) {
    unlink(res_filepath.c_str());
    unlink(req_filepath.c_str());
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after open", INFILE);
    std::exit(EXIT_FAILURE);
  }

  if (write(cgi_request, event.httpRequest.body().data(), event.httpRequest.body().size()) == -1) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after write", INFILE);
    Log::log()(true, "errno", strerror(errno), INFILE);
    std::exit(EXIT_FAILURE);
  }

  // int ret = lseek(cgi_request, 0, SEEK_SET);
  cgi_request = open(req_filepath.c_str(), O_RDONLY, 0644);
  if (cgi_request == -1) {
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after open", INFILE);
    std::exit(EXIT_FAILURE);
  }

  std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
  char*       argv[2]  = {0, 0};
  char*       envp[5]  = {0, 0, 0, 0, 0};
  argv[0]              = strdup(cgi_path.c_str());

  _setEnv(event.httpRequest, cgi_path, envp);
  _setFd(cgi_request, cgi_response);

  execve(cgi_path.c_str(), argv, envp);
  Log::log()(LOG_LOCATION, "(CGI) CALL FAILED after execve", INFILE);
  std::exit(EXIT_FAILURE);
}

void VirtualServer::_setEnv(const HttpRequest& http_request, const std::string& cgi_path, char** envp) const {
  std::string       server_protocol("SERVER_PROTOCOL=");
  std::string       request_method("REQUEST_METHOD=");
  std::string       path_info("PATH_INFO=");
  std::stringstream secret_header_for_test;
  ;

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
