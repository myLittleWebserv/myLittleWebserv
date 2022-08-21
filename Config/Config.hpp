/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:06:58 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/22 00:27:43 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <arpa/inet.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "Log.hpp"

struct LocationInfo {
  int                        maxBodySize;
  std::string                root;
  std::map<int, std::string> defaultErrorPages;
  std::vector<std::string>   allowedMethods;
  std::string                cgiExtension;
  std::string                cgiPath;
  std::string                indexPagePath;
  bool                       isAutoIndexOn;
  int                        redirStatus;
  std::string                redirPath;
};

struct ServerInfo {
  int                                 maxBodySize;
  std::string                         root;
  std::map<int, std::string>          defaultErrorPages;
  struct in_addr                      hostIp;
  int                                 hostPort;
  std::string                         serverName;
  std::map<std::string, LocationInfo> locations;
};

class Config {
  typedef std::vector<std::string>::iterator configIterator;

 private:
  std::vector<std::string>          _configContent;
  std::map<std::string, ServerInfo> _serverInfos;
  std::vector<int>                  _ports;
  void                              _readConfigFile(const std::string& confFile);
  void                              _printConfigContent();
  void                              _startParse();
  ServerInfo                        _parseServer(configIterator& it);
  LocationInfo                      _parseLocation(configIterator& it, const ServerInfo& serverInfo);
  std::map<int, std::string>        _parseDefaultErrorPage(const std::string& pages);
  std::vector<std::string>          _parseAllowedMethod(const std::string& value);
  void                              _parseRedirection(const std::string& value, LocationInfo& info);
  LocationInfo                      _init_locationInfo(const ServerInfo& serverInfo);
  std::vector<std::string>          _split(const std::string& str, const std::string& delimiter);
  std::pair<int, std::string>       _trimLeftTab(const std::string& str);
  std::string                       _trimLeftSpace(const std::string& str);

 public:
  Config(const std::string& confFile);
  virtual ~Config();
  std::vector<ServerInfo> getServerInfos();
  std::vector<int>        getPorts();
};

#endif