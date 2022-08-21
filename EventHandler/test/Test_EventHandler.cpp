#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../EventHandler.hpp"
#include "Config.hpp"
#include "Log.hpp"
#include "Router.hpp"

int main() {
  Config       config;
  Router       router(config);
  EventHandler event_handler(router);

  {
    Log::log().mark("Test EventHandler.addConnection");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;

    bzero(&addr, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(7777);
    addr.sin_addr.s_addr = INADDR_ANY;

    int ret;
    ret = bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    ret = listen(listen_fd, 100);

    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    if (ret == -1) {
      Log::log().mark("bind and listen");
      Log::log()(__FILE__, __LINE__, __func__, "bind or listen error", ALL);
      exit(1);
    }

    Log::log().getLogStream() << "Port : " << ntohs(addr.sin_port) << " Listen on fd : " << listen_fd << std::endl;

    Event* event = new Event(CONNECTION_REQUEST, listen_fd);

    event_handler.appendNewEventToChangeList(event->keventId, EVFILT_READ, EV_ADD, event);

    while (1) {
      event_handler.routeEvents();
      std::vector<Event*> v = event_handler.getRoutedEvents(2);
      for (auto i = v.begin(); i != v.end(); ++i) {
        Event& event = **i;

        if (event.type == HTTP_RESPONSE_WRITABLE) {
          // send to client
          Log::log()(LOG_LOCATION, "(send) Sending Http Response to client is done", ALL);
          event_handler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_DISABLE, NULL);
          // event_handler.removeConnection(event);
          delete &event;
        } else if (event.type == HTTP_REQUEST_READABLE) {
          // if cgi : callCgi
          // if not cgi : make http response
          event.type = HTTP_RESPONSE_WRITABLE;
          event_handler.appendNewEventToChangeList(event.keventId, EVFILT_WRITE, EV_ENABLE, &event);
          Log::log()(LOG_LOCATION, "(make) makeing Http Request to Http Response is done", ALL);
        } else if (event.type == CGI_RESPONSE_READABLE) {
          // make http response
          event.type = HTTP_RESPONSE_WRITABLE;
          event_handler.appendNewEventToChangeList(event.clientFd, EVFILT_WRITE, EV_ENABLE, &event);
          Log::log()(LOG_LOCATION, "(make) makeing Cgi Reponse to Http Response is done", ALL);
        }
      }
    }
  }

  return 0;
}