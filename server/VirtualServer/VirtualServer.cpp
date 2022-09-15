#include "VirtualServer.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <cstring>

#include "FileManager.hpp"
#include "ResponseFactory.hpp"
#include "Router.hpp"
#include "syscall.hpp"

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

ServerInfo& VirtualServer::getServerInfo() const { return _serverInfo; }

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (std::vector<Event*>::size_type i = 0; i < event_list.size(); i++) {
    Event& event = *event_list[i];
    _processEvent(event);
  }
}

void VirtualServer::_processEvent(Event& event) {
  LocationInfo& location_info = _findLocationInfo(event.httpRequest);
  switch (event.type) {
    case HTTP_REQUEST_READABLE:
      _processHttpRequestReadable(event, location_info);
      break;

    case HTTP_REQUEST_UPLOAD:
      _uploadFile(event, location_info);
      break;

    case HTTP_RESPONSE_DOWNLOAD:
      _downloadFile(event, location_info);
      break;

    case HTTP_RESPONSE_WRITABLE:
      _sendResponse(event);
      break;

    case CGI_RESPONSE_READABLE:
      _cgiResponseToHttpResponse(event, location_info);
      break;

    case BUFFER_FLUSH:
      _flushBuffer(event, location_info);

    default:
      break;
  }
}

void VirtualServer::_sendResponse(Event& event) {
  HttpResponse& response = *event.httpResponse;
  response.sendResponse(event.toSendFd);
  if (response.isConnectionClosed()) {
    Log::log()(LOG_LOCATION, "CLOSE BECAUSE CONNECTION CLOSED FOR SENDING");
    _eventHandler.removeConnection(event);
  } else if (response.isSendingEnd()) {
    _finishResponse(event);
  }
}

void VirtualServer::_finishResponse(Event& event) {
  Log::log()(true, "(DONE) sending Http Response", INFILE);
  Log::log()(true, "HTTP_RESPONSE_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, CONSOLE);
  if (event.httpRequest.isKeepAlive() && event.httpResponse->statusCode() != STATUS_BAD_REQUEST) {
    event.initialize();
    _eventHandler.disableWriteEvent(event.clientFd, &event);
    _eventHandler.enableReadEvent(event.clientFd, &event);
  } else {
    Log::log()(LOG_LOCATION, "CLOSE BECAUSE SENDING RESPONSE DONE");
    _eventHandler.removeConnection(event);
  }
  Log::log().increaseProcessedConnection();
  Log::log().printStatus();
  Log::log().mark();
}

void VirtualServer::_downloadFile(Event& event, LocationInfo& location_info) {
  // if (event.toRecvFd == -1) {  // ?
  //   event.type = HTTP_RESPONSE_WRITABLE;
  //   _eventHandler.enableWriteEvent(event.clientFd, &event);
  //   return;
  // }

  (void)location_info;
  HttpResponse& response    = *event.httpResponse;
  int           fd_received = event.toRecvFd;
  response.downloadResponse(event.toRecvFd, event.baseClock);
  // Log::log()(true, "downloadTotalSize", response.downloadTotalSize());

  switch (response.state()) {
    case HTTP_SENDING_TIME_OUT:
    case HTTP_SENDING_CONNECTION_CLOSED:
      FileManager::registerFileFdToClose(fd_received);
      _eventHandler.deleteReadEvent(fd_received, NULL);
      _eventHandler.removeConnection(event);
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE SENDING CONNECTION CLOSED in downloadFile");
      break;

    case HTTP_SENDING_STORAGE:
      FileManager::registerFileFdToClose(fd_received);
      event.type     = HTTP_RESPONSE_WRITABLE;
      event.toSendFd = event.clientFd;
      _eventHandler.deleteReadEvent(fd_received, NULL);
      _eventHandler.enableWriteEvent(event.clientFd, &event);

      Log::log()(true, "HTTP_RESPONSE_DOWNLOAD DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC,
                 CONSOLE);
      // Log::log().printHttpResponse(*event.httpResponse, INFILE);

    default:
      break;
  }
}

void VirtualServer::_flushBuffer(Event& event, LocationInfo& location_info) {
  ssize_t      file_size;
  HttpRequest& request     = event.httpRequest;
  int          tempfile_fd = event.toSendFd;
  request.uploadRequest(tempfile_fd, event.baseClock);

  switch (request.state()) {
    case HTTP_PARSING_TIME_OUT:
    case HTTP_PARSING_CONNECTION_CLOSED:
      FileManager::removeFile(TEMP_TRASH);
      FileManager::registerFileFdToClose(tempfile_fd);
      _eventHandler.deleteWriteEvent(tempfile_fd, NULL);
      _eventHandler.removeConnection(event);
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE PARSING CONNECTION CLOSED in flushSocket");
      break;

    case HTTP_PARSING_BAD_REQUEST:
      delete event.httpResponse;
      event.httpResponse = ResponseFactory::errorResponse(STATUS_BAD_REQUEST, request, location_info);
      FileManager::removeFile(TEMP_TRASH);
      _redirectUploadError(event);
      break;

    case HTTP_UPLOADING_DONE:
      file_size = ft::syscall::lseek(tempfile_fd, 0, SEEK_END);
      if (file_size > location_info.maxBodySize) {
        delete event.httpResponse;
        event.httpResponse = ResponseFactory::errorResponse(STATUS_PAYLOAD_TOO_LARGE, request, location_info);
      }
      FileManager::removeFile(TEMP_TRASH);
      FileManager::registerFileFdToClose(tempfile_fd);
      _redirectUploadError(event);
      Log::log()(true, "BUFFER_FLUSH           DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC,
                 CONSOLE);

    default:
      break;
  }
}

void VirtualServer::_uploadFile(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  int          fd_sent = event.toSendFd;
  int          fd_cgi_response;
  request.uploadRequest(event.toSendFd, event.baseClock);
  // Log::log()(true, "uploadedTotalSize", event.httpRequest.uploadedTotalSize());

  switch (request.state()) {
    case HTTP_PARSING_TIME_OUT:
    case HTTP_PARSING_CONNECTION_CLOSED:
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE PARSING CONNECTION CLOSED in uploadFile");
      _eventHandler.removeConnection(event);
      break;

    case HTTP_PARSING_BAD_REQUEST:
      delete event.httpResponse;
      event.httpResponse = ResponseFactory::errorResponse(STATUS_BAD_REQUEST, request, location_info);  // ?
      _redirectUploadError(event);
      break;

    case HTTP_UPLOADING_DONE:  // ?
      FileManager::registerFileFdToClose(fd_sent);
      _eventHandler.deleteWriteEvent(fd_sent, NULL);
      fd_cgi_response = _callCgi(event, location_info);
      if (fd_cgi_response != -1) {
        event.type     = CGI_RESPONSE_READABLE;
        event.toRecvFd = fd_cgi_response;
        _eventHandler.addReadEvent(event.toRecvFd, &event);
      } else {
        HttpResponse& response = *event.httpResponse;
        switch (response.type()) {
          case TYPE_POST:
            event.type     = HTTP_RESPONSE_WRITABLE;
            event.toSendFd = event.clientFd;
            _eventHandler.enableWriteEvent(event.clientFd, &event);
            break;

          case TYPE_FLUSH_ERR:  // CALL CGI ERROR
            event.type     = HTTP_RESPONSE_DOWNLOAD;
            event.toRecvFd = response.fileFd();
            _eventHandler.addReadEvent(event.toRecvFd, &event);
            break;

          default:
            throw "unexpected response type";
            break;
        }
      }
      Log::log()(true, "HTTP_REQUEST_UPLOAD    DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC,
                 CONSOLE);

    default:
      break;
  }
}

void VirtualServer::_redirectUploadError(Event& event) {
  HttpResponse& response    = *event.httpResponse;
  int           tempfile_fd = event.toSendFd;
  _eventHandler.deleteWriteEvent(tempfile_fd, NULL);

  switch (response.type()) {
    case TYPE_FLUSH:
      event.type     = HTTP_RESPONSE_WRITABLE;
      event.toSendFd = event.clientFd;
      _eventHandler.enableWriteEvent(event.clientFd, &event);
      break;

    case TYPE_FLUSH_ERR:
      event.type     = HTTP_RESPONSE_DOWNLOAD;
      event.toRecvFd = response.fileFd();
      _eventHandler.addReadEvent(event.toRecvFd, &event);
      break;

    default:
      throw "wrong routine of evnet";
      break;
  }
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  int          tempfile_fd;
  std::string  tempfile_path;
  int          fd_received = event.toRecvFd;

  // Log::log().printHttpRequest(event.httpRequest, INFILE);

  switch (request.method()) {
    case POST:
    case PUT:
      if (request.isCgi(location_info.cgiExtension)) {
        tempfile_path = TEMP_REQUEST_PREFIX + _intToString(event.clientFd);
        tempfile_fd   = ft::syscall::open(tempfile_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
        ft::syscall::fcntl(tempfile_fd, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));
        event.type     = HTTP_REQUEST_UPLOAD;
        event.toSendFd = tempfile_fd;
        _eventHandler.disableReadEvent(fd_received, &event);
        _eventHandler.addWriteEvent(tempfile_fd, &event);
        break;
      }

    default:
      event.httpResponse     = ResponseFactory::makeResponse(request, location_info);  // redir ?
      HttpResponse& response = *event.httpResponse;
      _eventHandler.disableReadEvent(fd_received, &event);

      switch (response.type()) {
        case TYPE_POST:
          event.type     = HTTP_REQUEST_UPLOAD;
          event.toSendFd = response.fileFd();
          _eventHandler.addWriteEvent(event.toSendFd, &event);
          break;

        case TYPE_GET:
          event.type     = HTTP_RESPONSE_DOWNLOAD;
          event.toRecvFd = response.fileFd();
          _eventHandler.addReadEvent(event.toRecvFd, &event);
          break;

        case TYPE_DEL:
          event.type     = HTTP_RESPONSE_WRITABLE;
          event.toSendFd = event.clientFd;
          _eventHandler.enableWriteEvent(event.clientFd, &event);
          break;

        case TYPE_FLUSH:
        case TYPE_FLUSH_ERR:
          event.type     = BUFFER_FLUSH;
          tempfile_fd    = ft::syscall::open(TEMP_TRASH, O_WRONLY | O_TRUNC | O_CREAT);
          event.toSendFd = tempfile_fd;
          ft::syscall::fcntl(tempfile_fd, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));
          _eventHandler.addWriteEvent(event.toSendFd, &event);

        default:
          break;
      }
  }

  Log::log()(true, "HTTP_REQUEST_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, CONSOLE);
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  event.httpResponse = ResponseFactory::makeResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_DOWNLOAD;
  event.httpResponse->setFileFd(event.toRecvFd);
  Log::log()(true, "(DONE) making Http Response using CGI RESPONSE", INFILE);
  Log::log()(true, "CGI_RESPONSE_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, CONSOLE);
}

LocationInfo& VirtualServer::_findLocationInfo(HttpRequest& httpRequest) {
  std::string                                   key;
  std::map<std::string, LocationInfo>::iterator found;

  key = httpRequest.uri();
  while (key != "/" && !key.empty()) {
    found = _serverInfo.locations.find(key);
    if (found != _serverInfo.locations.end()) {
      // Log::log()(true, "LocationInfo found: key", found->first, INFILE);
      return found->second;
    }
    key = key.substr(0, key.rfind('/'));
  }
  // Log::log()(true, "LocationInfo is default", INFILE);
  return _serverInfo.locations["/"];
}

int VirtualServer::_callCgi(Event& event, LocationInfo& location_info) {
  if (!event.httpRequest.isCgi(location_info.cgiExtension)) {
    return -1;
  }

  std::string fd           = _intToString(event.clientFd);
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;
  int         cgi_response = -1;

  pid_t pid = fork();
  if (pid == -1) {
    event.httpResponse = ResponseFactory::errorResponse(STATUS_INTERNAL_SERVER_ERROR, event.httpRequest, location_info);
    Log::log()(LOG_LOCATION, "(CGI) CALL FAILED", INFILE);
  } else if (pid == 0) {
    _execveCgi(event);  // child
  } else {              // parent
    event.cgiResponse.setPid(pid);
    cgi_response = ft::syscall::open(res_filepath.c_str(), O_RDONLY | O_TRUNC | O_CREAT, 0666);
    ft::syscall::fcntl(cgi_response, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));
    event.cgiResponse.setInfo(event.httpRequest);
    Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", INFILE);
  }
  return cgi_response;
}

void VirtualServer::_execveCgi(Event& event) {
  std::string fd           = _intToString(event.clientFd);
  std::string req_filepath = TEMP_REQUEST_PREFIX + fd;
  std::string res_filepath = TEMP_RESPONSE_PREFIX + fd;
  int         cgi_response = ::open(res_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  int         cgi_request  = ::open(req_filepath.c_str(), O_RDONLY | O_CREAT, 0666);

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
  // Log::log()(true, "server_protocol", server_protocol, INFILE);
  // Log::log()(true, "request_method", request_method, INFILE);
  // Log::log()(true, "path_info", path_info, INFILE);
  // Log::log()(true, "secret_header_for_test", secret_header_for_test.str(), INFILE);
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
