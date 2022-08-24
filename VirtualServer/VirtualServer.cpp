#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

VirtualServer::VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler)
    : _serverId(id), _serverInfo(info), _eventHandler(eventHandler) {}

std::string VirtualServer::_findLocation(HttpRequest& httpReuest) {
  (void)httpReuest;
  return "/";
}

void VirtualServer::start() {
  std::vector<Event*> event_list = _eventHandler.getRoutedEvents(_serverId);
  for (int i = 0; i < event_list.size(); i++) {
    Event&        event         = *event_list[i];
    std::string   location      = _findLocation(event.httpRequest);
    LocationInfo& location_info = _serverInfo.locations[location];

    switch (event.type) {
      case HTTP_REQUEST_READABLE:
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
        _sendResponse(event.clientFd, *event.httpResponse);
        if (event.httpRequest.isKeepAlive()) {
          event.type = HTTP_REQUEST_READABLE;
          delete event.httpResponse;
          event.httpResponse = NULL;
          Log::log()(LOG_LOCATION, "(FREE) event.httpResponse removed", ALL);
          _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_DISABLE, &event);
          _eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ENABLE, &event);
          event.httpRequest.initialize();
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
  // //  std::string cgi_path = getCgiPath(_serverInfo.locations); -> httpRequest에서 url 받아와서 찾아야 할 듯
  // int _pipe[2];  // pipe를 event에 넣어야 할 것 같다.
  //                // Waitpid Wnohang시 프로스세가 끝난지 한참 지나도 pid를 리턴 하는 것 확인
  //                // eventHandler에서 WNOHANG으로 pid를 확인해,
  // if (pipe(_pipe) == -1) {
  //   throw "error";
  // }
  // int env = 0;
  // env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  // env += setenv("REQUEST_METHOD", event.httpRequest.getMethod().c_str(), 1);
  // env += setenv("PATH_INFO", cgiPath.c_str(), 1);
  // if (env != 0) {
  //   throw "error";
  // }
  // fcntl(_pipe[WRITEEND], F_SETFL, O_NONBLOCK);
  // fcntl(_pipe[READEND], F_SETFL, O_NONBLOCK);
  // event.pid = fork();
  // if (event.pid == -1) {
  //   throw "500";                // 500번대 에러
  // } else if (event.pid == 0) {  // child
  //   int temp = open("temp/temp.txt", O_RDWR | O_CREAT, 0644);
  //   if (temp == -1) {
  //     exit(EXIT_FAILURE);
  //   }
  //   dup2(temp, STDIN_FILENO);
  //   dup2(_pipe[WRITEEND], STDOUT_FILENO);
  //   close(_pipe[READEND]);
  //   execve(cgi_path.c_str(), cgi_path.c_str(), NULL);
  //   exit(EXIT_FAILURE);
  // }
}

void VirtualServer::_sendResponse(int fd, HttpResponse& response) {
  send(fd, "hi\n", 3, 0);
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
