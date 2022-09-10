#include "VirtualServer.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstring>

#include "DataMove.hpp"
#include "FileManager.hpp"
#include "ResponseFactory.hpp"

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
      break;

    case HTTP_REQUEST_UPLOAD:
      _uploadFile(event, location_info);
      break;

    case CGI_RESPONSE_READABLE:
      _cgiResponseToHttpResponse(event, location_info);
      break;

    case HTTP_RESPONSE_WRITABLE:
      Log::log().printHttpResponse(*event.httpResponse, INFILE);
      event.httpResponse->sendResponse(event.toRecvFd, event.toSendFd);
      if (event.httpResponse->isConnectionClosed()) {
        _eventHandler.removeConnection(event);
      } else if (event.httpResponse->isSendingEnd()) {
        _finishResponse(event);
      }
      break;

    default:
      break;
  }
}

void VirtualServer::_finishResponse(Event& event) {
  if (event.cgiResponse.pid() != -1) {  // ?
    FileManager::removeFile(event.clientFd);
  }

  if (event.toRecvFd != -1) {
    close(event.toRecvFd);
  }

  Log::log()(true, "(DONE) sending Http Response", INFILE);
  Log::log()(true, "HTTP_RESPONSE_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
  if (event.httpRequest.isKeepAlive()) {
    event.initialize();
    _eventHandler.disableWriteEvent(event.toSendFd, &event);
    _eventHandler.enableReadEvent(event.toSendFd, &event);
  } else {
    _eventHandler.removeConnection(event);
  }
  Log::log().increaseProcessedConnection();
  Log::log().printStatus();
  Log::log().mark();
}

void VirtualServer::_uploadFile(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  request.uploadRequest(event.toRecvFd, event.toSendFd, event.baseClock);
  if (request.isRecvError()) {
    _eventHandler.removeConnection(event);
  } else if (request.isBadRequest()) {
    delete event.httpResponse;
    event.httpResponse = ResponseFactory::errorResponse(STATUS_BAD_REQUEST, request, location_info);
    event.type         = HTTP_RESPONSE_WRITABLE;
    _eventHandler.disableReadEvent(event.toRecvFd, &event);
    _eventHandler.enableWriteEvent(event.clientFd, &event);
    event.toRecvFd = event.httpResponse->fileFd();
  } else if (request.isUploadEnd()) {
    close(event.toSendFd);
    if (request.isCgi(location_info.cgiExtension) && _callCgi(event, location_info)) {
      event.type = CGI_RESPONSE_READABLE;
      _eventHandler.disableReadEvent(event.clientFd, &event);
      _eventHandler.addReadEvent(event.toRecvFd, &event);
      event.toSendFd = event.clientFd;
    } else {
      event.type = HTTP_RESPONSE_WRITABLE;
      _eventHandler.disableReadEvent(event.toRecvFd, &event);
      _eventHandler.enableWriteEvent(event.clientFd, &event);
      event.toRecvFd = -1;
    }
    Log::log()(true, "HTTP_REQUEST_UPLOAD    DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
  }
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  switch (request.method()) {
    case POST:
    case PUT:
      if (request.isCgi(location_info.cgiExtension)) {
        std::string req_filepath = TEMP_REQUEST_PREFIX + _intToString(event.clientFd);
        event.toSendFd           = ft::syscall::open(req_filepath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        event.type               = HTTP_REQUEST_UPLOAD;
      } else {
        event.httpResponse            = ResponseFactory::makeResponse(request, location_info);
        HttpResponseStatusCode status = event.httpResponse->statusCode();
        if (status == STATUS_OK || status == STATUS_CREATED) {
          event.toSendFd = event.httpResponse->fileFd();
          event.type     = HTTP_REQUEST_UPLOAD;
        } else {  // error
          _eventHandler.disableReadEvent(event.toRecvFd, &event);
          _eventHandler.enableWriteEvent(event.toSendFd, &event);
          event.toRecvFd = event.httpResponse->fileFd();
          event.type     = HTTP_RESPONSE_WRITABLE;
        }
      }
      break;

    default:
      _eventHandler.disableReadEvent(event.toRecvFd, &event);
      _eventHandler.enableWriteEvent(event.toSendFd, &event);
      event.httpResponse = ResponseFactory::makeResponse(request, location_info);
      event.toRecvFd     = event.httpResponse->fileFd();
      event.type         = HTTP_RESPONSE_WRITABLE;
      break;
  }
  Log::log()(true, "HTTP_REQUEST_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  event.httpResponse = ResponseFactory::makeResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_WRITABLE;
  _eventHandler.deleteReadEvent(event.keventId, NULL);
  _eventHandler.enableWriteEvent(event.toSendFd, &event);
  Log::log()(true, "(DONE) making Http Response using CGI RESPONSE", INFILE);
  Log::log()(true, "CGI_RESPONSE_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
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

bool VirtualServer::_callCgi(Event& event, LocationInfo& location_info) {
  std::string fd           = _intToString(event.toSendFd);
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;

  pid_t pid = fork();
  if (pid == -1) {
    event.httpResponse = ResponseFactory::errorResponse(STATUS_INTERNAL_SERVER_ERROR, event.httpRequest, location_info);
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", INFILE);
    return false;
  } else if (pid == 0) {
    _execveCgi(event);  // child
  } else {              // parent
    event.cgiResponse.setPid(pid);
    int cgi_response = ft::syscall::open(res_filepath.c_str(), O_RDONLY | O_CREAT, 0644);
    ft::syscall::fcntl(cgi_response, F_SETFL, O_NONBLOCK);
    event.toRecvFd = cgi_response;
    event.keventId = cgi_response;
    event.cgiResponse.setInfo(event.httpRequest);
    event.cgiResponse.getLine().setFd(cgi_response);
  }
  Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", INFILE);
  return true;
}

void VirtualServer::_execveCgi(Event& event) {
  event.baseClock          = clock();
  std::string fd           = _intToString(event.clientFd);
  std::string req_filepath = TEMP_REQUEST_PREFIX + fd;
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;
  int         cgi_response = open(res_filepath.c_str(), O_WRONLY | O_CREAT, 0644);
  int         cgi_request  = open(req_filepath.c_str(), O_RDONLY, 0644);

  if (cgi_request == -1 || cgi_response == -1) {
    unlink(req_filepath.c_str());
    unlink(res_filepath.c_str());
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
