#ifndef MYLITTLEWEBSERV_VIRTUALSERVER_HPP
#define MYLITTLEWEBSERV_VIRTUALSERVER_HPP

//have to include a header for struct ServerInfo

class VirtualServer {
  private:
    int _serverId;
    ServerInfo _serverInfo;

  public:
    VirtualServer(int id, ServerInfo info);
    void start();
};


#endif
