/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mypark <mypark@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:06:58 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/25 23:18:00 by mypark           ###   ########.fr       */
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
  struct in_addr             hostIp;
  int                        hostPort;
  std::string                serverName;
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
  // Types
  typedef std::vector<std::string>::iterator                  configIterator;
  typedef std::map<std::string, LocationInfo>::const_iterator locationInfoConstIterator;
  typedef std::vector<ServerInfo>::const_iterator             serverInfoConstIterator;
  typedef std::map<int, std::string>::const_iterator          defaultErrorPagesConstIterator;

  // Member Variable
 private:
  std::vector<std::string> _configContent;
  std::vector<ServerInfo>  _serverInfos;
  std::vector<int>         _ports;

  // Method
 private:
  void         _readConfigFile(const std::string& confFile);
  void         _printConfigContent();
  void         _startParse();
  ServerInfo   _parseServer(configIterator& it, const configIterator& end);
  LocationInfo _parseLocation(configIterator& it, const ServerInfo& serverInfo);
  void         _parseLocationInfoToken(LocationInfo& info, const std::string& identifier, const std::string& value);
  std::map<int, std::string>  _parseDefaultErrorPage(const std::string& pages);
  std::vector<std::string>    _parseAllowedMethod(const std::string& value);
  void                        _parseRedirection(const std::string& value, LocationInfo& info);
  LocationInfo                _init_locationInfo(const ServerInfo& serverInfo);
  std::vector<std::string>    _split(const std::string& str, const std::string& delimiter);
  std::pair<int, std::string> _trimLeftTab(const std::string& str);
  std::string                 _trimLeftSpace(const std::string& str);
  void                        _serverInfoString(std::stringstream& _ss, const ServerInfo& info);
  void                        _locatinInfoString(std::stringstream& _ss, const LocationInfo& info);
  void                        _parsedConfigResult();
  void                        _setPorts();

  // Constructor
 public:
  Config(const std::string& confFile);

  // Destructor
 public:
  virtual ~Config();

  // Interface
 public:
  std::vector<ServerInfo>& getServerInfos();
  std::vector<int>&        getPorts();
};

#endif
