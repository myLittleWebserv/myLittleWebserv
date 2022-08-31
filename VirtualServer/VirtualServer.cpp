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

#define READEND 0
#define WRITEEND 1

void VirtualServer::_callCgi(Event& event) {
  std::string cgi_path = _findLocationInfo(event.httpRequest).cgiPath;
  if (event.pid == waitpid(event.pid, NULL, WNOHANG)) {
    std::stringstream ss;
    ss << event.pid;
    std::string pid      = ss.str();
    std::string filepath = "temp/" + pid;
    unlink(filepath.c_str());
    //Make cgi response
    event.type = CGI_RESPONSE_READABLE;
    _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_DISABLE, &event);
//    _eventHandler.appendNewEventToChangeList(event.pipe[READEND], EVFILT_READ, EV_ENABLE, &event);
    return;
  }
  if (waitpid(event.pid, NULL, WNOHANG) == -1)
    throw "error";
  int env = 0;
  env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  env += setenv("REQUEST_METHOD", "POST", 1); // 현재 httpRequest에서 method()가 enum을 리턴해서 케이스별로 문자열로 넣어줘야함
  env += setenv("PATH_INFO", cgi_path.c_str(), 1);
  if (env != 0) {
    throw "error";
  }
//  fcntl(event.pipe[WRITEEND], F_SETFL, O_NONBLOCK);
//  fcntl(event.pipe[READEND], F_SETFL, O_NONBLOCK);
  event.pid = fork();
  if (event.pid == -1) {
    throw "500";                // 500번대 에러
  } else if (event.pid == 0) {  // child
    std::stringstream ss;
    ss << event.pid;
    std::string pid      = ss.str();
    std::string filepath = "temp/" + pid;
    int         temp     = open(filepath.c_str(), O_RDWR | O_CREAT, 0644);
    if (temp == -1) {
      exit(EXIT_FAILURE);
    }
    dup2(temp, STDIN_FILENO);
    char** argv = new char*[2];
    argv[0]     = const_cast<char*>(cgi_path.c_str());
    argv[1]     = NULL;
    execve(cgi_path.c_str(), argv, NULL);
    exit(EXIT_FAILURE);
  } else {  // parent
//    close(event.pipe[WRITEEND]);
  }
}
//분기를 계속 나눠서 읽기 // 쓰기가 끝나지 않은 때, 끝난 때로 구분
//해야하는 일 -> httpRequest를 cgi에 넣어서 cgiResponse를 만들어야함
//httpRequest를 cgi에 넣는 방법 -> cgi에 넣을 수 있는 파일을 만들어야함
//------
//httpRequest 통째로 파일에 write
//------
//write가 끝나면 execve


void VirtualServer::_sendResponse(int fd, HttpResponse& response) {
  //(void)response;
  // send(fd, "hi\n", 3, 0);
  send(fd, response.headerToString().c_str(), response.headerToString().size(), 0);
  send(fd, response.body().data(), response.body().size(), 0);
  Log::log()(LOG_LOCATION, "(SYSCALL) send HttpResponse to client ", ALL);

  // std::string response_str = response.getResponse();
  // int         sent_length  = response.sentLength;  // httpResponse 내부에 sent_length 넣을 까 요?
  // if (response_str.length() == sent_length) {
  //   return;
  // }
  // int len;
  // if ((len = send(fd, response_str.c_str() + sent_length, response_str.size() - sent_length, 0)) == -1) {
  //   throw "send() error!";
  // }
  // response.sentLength += len;
}

ServerInfo& VirtualServer::getServerInfo() const { return _serverInfo; }
