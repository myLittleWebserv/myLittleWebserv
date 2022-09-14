#ifndef MYLITTLEWEBSERV_VIRTUALSERVER_HPP
#define MYLITTLEWEBSERV_VIRTUALSERVER_HPP

#include "Config.hpp"
#include "EventHandler.hpp"

class HttpResponse;

class VirtualServer {
 private:
  int           _serverId;
  ServerInfo&   _serverInfo;
  EventHandler& _eventHandler;

  void          _processEvent(Event& event);
  bool          _callCgi(Event& event, LocationInfo& location_info);
  void          _execveCgi(Event& event);
  void          _sendResponse(int fd, HttpResponse& response);
  LocationInfo& _findLocationInfo(HttpRequest& httpRequest);
  std::string   _intToString(int integer);
  void          _setEnv(const HttpRequest& http_request, const std::string& cgi_path, char** envp) const;
  void          _setFd(int request, int response) const;
  void          _finishResponse(Event& event);
  void          _sendResponse(Event& event);
  void          _processHttpRequestReadable(Event& event, LocationInfo& location_info);
  void          _cgiResponseToHttpResponse(Event& event, LocationInfo& location_info);
  void          _uploadFile(Event& event, LocationInfo& location_info);
  void          _downloadFile(Event& event, LocationInfo& location_info);
  void          _flushBuffer(Event& event, LocationInfo& location_info);
  void          _redirectUploadError(Event& event);
  void          _directUploadToClient(Event& event);
  void          _directUploadToDownload(Event& event, int fd);
  void          _directClientToUpload(Event& event, int fd);
  void          _directClientToDownload(Event& event, int fd);
  void          _directDownloadToClient(Event& event);

 public:
  VirtualServer(int id, ServerInfo& info, EventHandler& eventHandler);
  void        start();
  ServerInfo& getServerInfo() const;
};

#endif
