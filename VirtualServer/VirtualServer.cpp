#include "VirtualServer.hpp"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

VirtualServer::VirtualServer(int id, ServerInfo info) : _serverId(id), _serverInfo(info) {}

void VirtualServer::start(EventHandler& eventHandler) {
  std::vector<Event*> event_list = eventHandler.getRoutedEvents(_serverId);
  for (int i = 0; i < event_list.size(); i++) {
    Event& event = *event_list[i];
    switch (event.type) {
      case HTTP_REQUEST_READABLE:
        if (event.httpRequest.isCgi(_serverInfo.locations)) {
          callCgi(event);
          break;
        }
        event.httpResponse = HttpResponse(event.httpRequest);
        event.type = HTTP_RESPONSE_WRITABLE;
        eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ADD | EV_DISABLE, &event);
        eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ADD | EV_ENABLE, &event);
        break;
      case CGI_RESPONSE_READABLE:
        event.httpResponse = HttpResponse(event.cgiResponse));
        event.type = HTTP_RESPONSE_WRITABLE;
        eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ADD | EV_DISABLE, &event);
        eventHandler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ADD | EV_ENABLE, &event);
        break;
      case HTTP_RESPONSE_WRITABLE:
        sendResponse(event.keventId, *event.httpResponse);
        if (!event.httpRequest.isKeepAlive()) {
          eventHandler.removeConnection(event);
        } // 조건문 추가
        delete &event;
        eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_READ, EV_ADD | EV_DISABLE, NULL);
        eventHandler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ADD | EV_DISABLE, NULL);
        break;
      default:
        break;
    }
  }
}

#define READEND 0
#define WRITEEND 1

void VirtualServer::callCgi(Event& event) {
  //  std::string cgi_path = getCgiPath(_serverInfo.locations);
  int _pipe[2]; // pipe를 event에 넣어야 할 것 같다. Waitpid Wnohang시 프로스세가 끝난지 한참 지나도 pid를 리턴 하는 것 확인
  if (pipe(_pipe) == -1) {
    throw "error";
  }
  int env = 0;
  env += setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  env += setenv("REQUEST_METHOD", event.httpRequest.getMethod().c_str(), 1); // METHOD have to be decided
  env += setenv("PATH_INFO", cgiPath.c_str(), 1);
  if (env != 0) {
    throw "error";
  }
  fcntl(_pipe[WRITEEND], F_SETFL, O_NONBLOCK);
  fcntl(_pipe[READEND], F_SETFL, O_NONBLOCK);
  event.pid = fork();
  if (event.pid == -1) {
    throw "500"; // 500번대 에러
  } else if (event.pid == 0) {  // child
    int temp = open("temp/temp.txt", O_RDWR | O_CREAT, 0644);
    if (temp == -1) {
      exit(EXIT_FAILURE);
    }
    dup2(temp, STDIN_FILENO);
    dup2(_pipe[WRITEEND], STDOUT_FILENO);
    close(_pipe[READEND]);
    execve(cgi_path.c_str(), cgi_path.c_str(), NULL);
    exit(EXIT_FAILURE);
  }
}

void sendResponse(int fd, HttpResponse& response) {
  std::string response_str = response.getResponse();
  int sent_length = response.sentLength; //httpResponse 내부에 sent_length 넣을 까 요?
  if (response_str.length() == sent_length) {
    return;
  }
  int len;
  if ((len = send(fd, response_str.c_str() + sent_length, response_str.size() - sent_length, 0)) == -1) {
    throw "send() error!";
  }
  response.sentLength += len;
}

ServerInfo VirtualServer::getServerInfo() { return _serverInfo; }

