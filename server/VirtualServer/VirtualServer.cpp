#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

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

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (std::vector<Event*>::size_type i = 0; i < event_list.size(); i++) {
    Event&        event         = *event_list[i];
    LocationInfo& location_info = _findLocationInfo(event.httpRequest);

    switch (event.type) {
      case HTTP_REQUEST_READABLE:
        Log::log().printHttpRequest(event.httpRequest, ALL);
        if (event.httpRequest.isCgi(location_info.cgiExtension)) {
          _callCgi(event);
          break;
        }
        event.httpResponse = new HttpResponse(event.httpRequest, location_info);
        event.type         = HTTP_RESPONSE_WRITABLE;
        _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
        _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
        Log::log()(LOG_LOCATION, "(DONE) making Http Response", ALL);
        break;

      case CGI_RESPONSE_READABLE:
        event.httpResponse = new HttpResponse(event.cgiResponse, location_info);
        event.type         = HTTP_RESPONSE_WRITABLE;
        _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
        _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ENABLE, &event);
        Log::log()(LOG_LOCATION, "(DONE) making Http Response", ALL);
        break;

      case HTTP_RESPONSE_WRITABLE:
        Log::log().printHttpResponse(*event.httpResponse, ALL);
        _sendResponse(event.clientFd, *event.httpResponse);
        if (event.httpResponse->sentLength() != (int)event.httpResponse->body().size()) {
          return;
        }
        if (event.httpRequest.isKeepAlive()) {
          event.type = HTTP_REQUEST_READABLE;
          event.httpRequest.initialize();
          // event.cgiResponse.initialize();
          delete event.httpResponse;
          event.httpResponse = NULL;
          Log::log()(LOG_LOCATION, "(FREE) event.httpResponse removed", ALL);
          _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_DISABLE, &event);
          _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ENABLE, &event);
        } else {  // 조건문 추가
          _eventHandler.removeConnection(event);
          delete &event;
          Log::log()(LOG_LOCATION, "(FREE) event removed", ALL);
        }
        break;

      default:
        break;
    }
  }
}

void VirtualServer::_callCgi(Event& event) {
  if (event.pid != -1) {
    return;
  } // fd로 파일 이름 관리 keventId
  event.pid = fork();
  if (event.pid == -1) {
    throw "500";  // 500번대 에러
  }
  if (event.pid == 0) {  // child
    std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
    _setEnv(event, cgi_path);
    std::string pid          = _pidToString(getpid());
    std::string res_filepath = "temp/response" + pid;
    int         response     = open(res_filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (response == -1) {
      exit(EXIT_FAILURE);
    }
    std::string req_filepath = "temp/request" + pid;
    int         request      = open(req_filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (request == -1) {
      unlink(res_filepath.c_str());
      exit(EXIT_FAILURE);
    }
    write(request, event.httpRequest.storage().getData(), event.httpRequest.storage().size());
    dup2(request, STDIN_FILENO);
    close(request);
    dup2(response, STDOUT_FILENO);
    close(response);
    char** argv = new char*[2];
    argv[0]     = const_cast<char*>(cgi_path.c_str());
    argv[1]     = NULL;
    execve(cgi_path.c_str(), argv, NULL);
    exit(EXIT_FAILURE);
  } else {  // Open을 위해 pid를 알아야 해서 그냥 두번 여는게 낫겠다 싶었습니다.
    std::string pid          = _pidToString(event.pid);
    std::string res_filepath = "temp/response" + pid;
    int         response     = open(res_filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (response == -1) {
      exit(EXIT_FAILURE);
    }
    event.keventId = response;
    event.type = CGI_RESPONSE_READABLE;
    _eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_READ, EV_DISABLE, &event);
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ENABLE, &event);
  }
}

void VirtualServer::_setEnv(Event& event, const std::string& cgi_path) const {
  std::string method;
  switch (event.httpRequest.method()) {
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
      throw "500";
  }
  int env = 0;
  env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  env += setenv("REQUEST_METHOD", method.c_str(), 1);
  env += setenv("PATH_INFO", cgi_path.c_str(), 1);
  if (env != 0) {
    throw "500";
  }
}

std::string VirtualServer::_pidToString(int pid) {
  std::stringstream ss;
  ss << pid;
  std::string pidstr = ss.str();
  return pidstr;
}

void VirtualServer::_sendResponse(int fd, HttpResponse& response) {
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
