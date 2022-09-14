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

    case HTTP_RESPONSE_DOWNLOAD:
      _downloadFile(event, location_info);
      break;

    case HTTP_RESPONSE_WRITABLE:
      Log::log().printHttpResponse(*event.httpResponse, INFILE);
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
  Log::log()(true, "HTTP_RESPONSE_WRITABLE DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
  if (event.httpRequest.isKeepAlive()) {
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
  if (event.toRecvFd == -1) {  // ?
    event.type = HTTP_RESPONSE_WRITABLE;
    _eventHandler.enableWriteEvent(event.clientFd, &event);
    return;
  }
  (void)location_info;
  HttpResponse& response = *event.httpResponse;
  response.downloadResponse(event.toRecvFd, event.baseClock);
  // Log::log()(true, "downloadTotalSize", response.downloadTotalSize());

  switch (response.state()) {
    case HTTP_SENDING_TIME_OUT:
    case HTTP_SENDING_CONNECTION_CLOSED:
      FileManager::registerFileFdToClose(event.toRecvFd);
      _eventHandler.deleteReadEvent(event.toRecvFd, NULL);
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE PARSING CONNECTION CLOSED in uploadFile");
      _eventHandler.removeConnection(event);
      break;

    case HTTP_SENDING_STORAGE:
      _directDownloadToClient(event);
      Log::log()(true, "HTTP_RESPONSE_DOWNLOAD DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);

    default:
      break;
  }
}

void VirtualServer::_flushBuffer(Event& event, LocationInfo& location_info) {
  ssize_t      file_size;
  HttpRequest& request = event.httpRequest;
  request.uploadRequest(event.toSendFd, event.baseClock);

  switch (request.state()) {
    case HTTP_PARSING_TIME_OUT:
    case HTTP_PARSING_CONNECTION_CLOSED:
      // FileManager::removeFile(TEMP_TRASH);
      FileManager::registerFileFdToClose(event.toSendFd);
      _eventHandler.deleteWriteEvent(event.toSendFd, NULL);
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE PARSING CONNECTION CLOSED in flushSocket");
      _eventHandler.removeConnection(event);
      break;

    case HTTP_PARSING_BAD_REQUEST:
      delete event.httpResponse;
      event.httpResponse = ResponseFactory::errorResponse(STATUS_BAD_REQUEST, request, location_info);
      // FileManager::removeFile(TEMP_TRASH);
      _redirectUploadError(event);
      break;

    case HTTP_UPLOADING_DONE:
      file_size = ft::syscall::lseek(event.toSendFd, 0, SEEK_END);
      if (file_size > location_info.maxBodySize) {
        delete event.httpResponse;
        event.httpResponse = ResponseFactory::errorResponse(STATUS_PAYLOAD_TOO_LARGE, request, location_info);
      }
      // FileManager::removeFile(TEMP_TRASH);
      FileManager::registerFileFdToClose(event.toSendFd);
      _redirectUploadError(event);
      Log::log()(true, "BUFFER_FLUSH           DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);

    default:
      break;
  }
}

void VirtualServer::_uploadFile(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  request.uploadRequest(event.toSendFd, event.baseClock);
  Log::log()(true, "uploadedTotalSize", event.httpRequest.uploadedTotalSize());

  switch (request.state()) {
    case HTTP_PARSING_TIME_OUT:
    case HTTP_PARSING_CONNECTION_CLOSED:
      FileManager::registerFileFdToClose(event.toSendFd);
      _eventHandler.deleteWriteEvent(event.toSendFd, NULL);
      Log::log()(LOG_LOCATION, "CLOSE BECAUSE PARSING CONNECTION CLOSED in uploadFile");
      _eventHandler.removeConnection(event);
      break;

    case HTTP_PARSING_BAD_REQUEST:
      delete event.httpResponse;
      event.httpResponse = ResponseFactory::errorResponse(STATUS_BAD_REQUEST, request, location_info);
      _redirectUploadError(event);
      break;

    case HTTP_UPLOADING_DONE:
      if (request.isCgi(location_info.cgiExtension) && _callCgi(event, location_info)) {
        // _directUploadToDownload(event, event.toRecvFd);
        Log::log()(true, "event.toSendFd", event.toSendFd);
        Log::log()(true, "event.toRecvFd", event.toRecvFd);
      } else {
        _directUploadToClient(event);
      }
      Log::log()(true, "HTTP_REQUEST_UPLOAD    DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);

    default:
      break;
  }
}

void VirtualServer::_redirectUploadError(Event& event) {
  _eventHandler.deleteWriteEvent(event.toSendFd, NULL);
  if (event.httpResponse->fileFd() == -1) {
    event.type = HTTP_RESPONSE_WRITABLE;
    _eventHandler.enableWriteEvent(event.clientFd, &event);
    event.setDataFlow(event.httpResponse->fileFd(), event.clientFd);
  } else {
    event.type = HTTP_RESPONSE_DOWNLOAD;
    _eventHandler.addReadEvent(event.httpResponse->fileFd(), &event);
    event.setDataFlow(event.httpResponse->fileFd(), event.clientFd);
  }
}

void VirtualServer::_directDownloadToClient(Event& event) {
  event.type = HTTP_RESPONSE_WRITABLE;
  _eventHandler.deleteReadEvent(event.toRecvFd, NULL);
  _eventHandler.enableWriteEvent(event.clientFd, &event);
  event.setDataFlow(event.toRecvFd, event.clientFd);
  event.httpResponse->setFileFd(event.toRecvFd);  // ?
}

void VirtualServer::_directUploadToClient(Event& event) {
  event.type = HTTP_RESPONSE_WRITABLE;
  _eventHandler.enableWriteEvent(event.clientFd, &event);
  _eventHandler.deleteWriteEvent(event.toSendFd, NULL);
  event.setDataFlow(-1, event.clientFd);
}

void VirtualServer::_directClientToUpload(Event& event, int fd) {  // -1 (x)
  event.type = HTTP_REQUEST_UPLOAD;
  event.setDataFlow(event.clientFd, fd);
  _eventHandler.addWriteEvent(event.toSendFd, &event);
  _eventHandler.disableReadEvent(event.clientFd, &event);
}

void VirtualServer::_directClientToDownload(Event& event, int fd) {
  event.type = HTTP_RESPONSE_DOWNLOAD;
  event.setDataFlow(fd, event.clientFd);
  _eventHandler.addReadEvent(event.toRecvFd, &event);
  _eventHandler.disableReadEvent(event.clientFd, &event);
}

void VirtualServer::_directUploadToDownload(Event& event, int fd) {  // -1 (x)
  event.type = CGI_RESPONSE_READABLE;
  FileManager::registerFileFdToClose(event.toSendFd);
  _eventHandler.deleteWriteEvent(event.toSendFd, NULL);
  _eventHandler.addReadEvent(fd, &event);
  event.setDataFlow(fd, event.clientFd);
}

void VirtualServer::_processHttpRequestReadable(Event& event, LocationInfo& location_info) {
  HttpRequest& request = event.httpRequest;
  int          tempfile_fd;
  std::string  tempfile_path;
  switch (request.method()) {
    case POST:
    case PUT:
      if (request.isCgi(location_info.cgiExtension)) {
        tempfile_path = TEMP_REQUEST_PREFIX + _intToString(event.clientFd);
        tempfile_fd   = ft::syscall::open(tempfile_path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
        ft::syscall::fcntl(tempfile_fd, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));
        _directClientToUpload(event, tempfile_fd);
        break;
      } else {
        event.httpResponse            = ResponseFactory::makeResponse(request, location_info);  // redir ?
        HttpResponseStatusCode status = event.httpResponse->statusCode();
        if (status == STATUS_OK || status == STATUS_CREATED) {
          _directClientToUpload(event, event.httpResponse->fileFd());
        } else {
          tempfile_fd = ft::syscall::open(TEMP_TRASH, O_WRONLY | O_TRUNC | O_CREAT);
          ft::syscall::fcntl(tempfile_fd, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));
          _directClientToUpload(event, tempfile_fd);
          event.type = BUFFER_FLUSH;
        }
        break;
      }

    default:  // GET HEAD DELETE NOT_IMPL ERROR
      event.httpResponse = ResponseFactory::makeResponse(request, location_info);
      if (event.httpResponse->fileFd() != -1 && event.httpResponse->contentLength() != 0) {  // fd == -1 ?
        _directClientToDownload(event, event.httpResponse->fileFd());
      } else {
        event.type = HTTP_RESPONSE_WRITABLE;
        event.httpResponse->setState(HTTP_SENDING_STORAGE);
        event.setDataFlow(-1, event.clientFd);
        _eventHandler.disableReadEvent(event.clientFd, &event);
        _eventHandler.enableWriteEvent(event.clientFd, &event);
      }
      break;
  }

  Log::log()(true, "HTTP_REQUEST_READABLE  DONE TIME", (double)(clock() - event.baseClock) / CLOCKS_PER_SEC, ALL);
}

void VirtualServer::_cgiResponseToHttpResponse(Event& event, LocationInfo& location_info) {
  event.httpResponse = ResponseFactory::makeResponse(event.cgiResponse, location_info);
  event.type         = HTTP_RESPONSE_DOWNLOAD;
  event.httpResponse->setFileFd(event.toRecvFd);  //?
  // _eventHandler.deleteReadEvent(event.toRecvFd, NULL);
  // _eventHandler.enableWriteEvent(event.clientFd, &event);
  // event.setDataFlow(event.toRecvFd, event.clientFd);
  // event.httpResponse->setFileFd(event.toRecvFd);
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
  std::string fd           = _intToString(event.clientFd);
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
    int cgi_response = ft::syscall::open(res_filepath.c_str(), O_RDONLY | O_TRUNC | O_CREAT, 0666);
    ft::syscall::fcntl(cgi_response, F_SETFL, O_NONBLOCK, Router::ServerSystemCallException("fcntl"));

    _directUploadToDownload(event, cgi_response);
    event.cgiResponse.setInfo(event.httpRequest);
    Log::log()(LOG_LOCATION, "(CGI) CALL SUCCESS", INFILE);
  }
  return true;
}

void VirtualServer::_execveCgi(Event& event) {
  event.baseClock          = clock();
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
