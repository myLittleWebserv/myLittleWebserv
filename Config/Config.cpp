/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/20 13:37:55 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/22 23:21:15 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// TODO : allowedMethods에 이상한 애들 들어왔으면 튕기기
// TODO : location에 서버의 정보 포함하기. (서버 주소, 포트, 서버 이름)
// TODO : listen host:port의 host는 옵셔널임. 있으면 해당 ip의 해당 포트만 받겠다는 뜻이고, 없으면 해당 포트번호로
// 들어오는 요청에 대해 모두 다 처리한다는 뜻.

#include "Config.hpp"

Config::Config(const std::string& confFile) {
  _readConfigFile(confFile);
  _startParse();
  _setPorts();
  _parsedConfigResult();
}

Config::~Config() {}

void Config::_readConfigFile(const std::string& confFile) {
  std::ifstream _file(confFile);

  if (_file.is_open()) {
    std::string _buffer;
    while (std::getline(_file, _buffer)) {
      _configContent.push_back(_buffer);
    }
  } else {
    std::cout << "ERROR: cannot open " << confFile << std::endl;
    std::exit(1);
  }
}

void Config::_printConfigContent() {
  for (configIterator it = _configContent.begin(); it != _configContent.end(); ++it) {
    std::cout << *it << std::endl;
  }
}

void Config::_startParse() {
  configIterator it = _configContent.begin();
  while (it != _configContent.end()) {
    if (*it == "server") {
      ServerInfo serverInfo = _parseServer(++it);
      _serverInfos.push_back(serverInfo);
    } else {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file", ALL);
      std::exit(1);
    }
  }
}

ServerInfo Config::_parseServer(configIterator& it) {
  ServerInfo _server_info;

  while (*it != "\n") {
    std::pair<int, std::string> _trimmed = _trimLeftTab(*it);
    if (_trimmed.first != 1) {
      break;
    }
    std::vector<std::string> _splitted = _split(_trimmed.second, ":");
    if (_splitted.size() != 2) {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file");
      std::exit(1);
    }
    std::string _identifier = _splitted[0];
    std::string _value      = _trimLeftSpace(_splitted[1]);
    if (_identifier == "client_max_body_size") {
      _server_info.maxBodySize = std::stoi(_value);
    } else if (_identifier == "root") {
      _server_info.root = _value;
    } else if (_identifier == "default_error_page") {
      _server_info.defaultErrorPages = _parseDefaultErrorPage(_value);
    } else if (_identifier == "host") {
      inet_aton(_value.c_str(), &_server_info.hostIp);
    } else if (_identifier == "port") {
      _server_info.hostPort = std::stoi(_value);
    } else if (_identifier == "server_name") {
      _server_info.serverName = _value;
    } else if (_identifier == "location") {
      _server_info.locations[_value] = _parseLocation(++it, _server_info);
      continue;
    } else {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file" + _identifier + _value, ALL);
      std::exit(1);
    }
    it++;
  }
  return _server_info;
}

LocationInfo Config::_parseLocation(configIterator& it, const ServerInfo& serverInfo) {
  LocationInfo _location_info = _init_locationInfo(serverInfo);
  while (*it != "\n") {
    std::pair<int, std::string> _trimmed = _trimLeftTab(*it);
    if (_trimmed.first != 2) {
      break;
    }
    std::vector<std::string> _splitted = _split(_trimmed.second, ":");
    if (_splitted.size() != 2) {
      if (_trimmed.second == "autoindex") {
        _location_info.isAutoIndexOn = true;
        it++;
        continue;
      }
      std::exit(1);
    }
    std::string _identifier = _splitted[0];
    std::string _value      = _trimLeftSpace(_splitted[1]);
    _parseLocationInfoToken(_location_info, _identifier, _value);
    it++;
  }
  return _location_info;
}

void Config::_parseLocationInfoToken(LocationInfo& info, const std::string& identifier, const std::string& value) {
  if (identifier == "client_max_body_size") {
    info.maxBodySize = std::stoi(value);
  } else if (identifier == "root") {
    info.root = value;
  } else if (identifier == "default_error_page") {
    info.defaultErrorPages = _parseDefaultErrorPage(value);
  } else if (identifier == "allowed_method") {
    info.allowedMethods = _parseAllowedMethod(value);
  } else if (identifier == "cgi_extension") {
    info.cgiExtension = value;
  } else if (identifier == "cgi_path") {
    info.cgiPath = value;
  } else if (identifier == "redirection") {
    _parseRedirection(value, info);
  } else if (identifier == "index") {
    info.indexPagePath = value;
  } else {
    std::cout << "ERROR: invalid config file" << std::endl;
    Log::log()(LOG_LOCATION, "ERROR: invalid config file" + identifier + value, ALL);
    std::exit(1);
  }
}

LocationInfo Config::_init_locationInfo(const ServerInfo& serverInfo) {
  LocationInfo _location_info;
  _location_info.isAutoIndexOn     = false;
  _location_info.root              = serverInfo.root;
  _location_info.defaultErrorPages = serverInfo.defaultErrorPages;
  _location_info.maxBodySize       = serverInfo.maxBodySize;
  _location_info.hostIp            = serverInfo.hostIp;
  _location_info.hostPort          = serverInfo.hostPort;
  _location_info.serverName        = serverInfo.serverName;
  return _location_info;
}

std::vector<std::string> Config::_split(const std::string& str, const std::string& delimiter) {
  std::vector<std::string> _result;
  std::string::size_type   _prev_pos = 0;
  std::string::size_type   _pos      = 0;

  while ((_pos = str.find(delimiter, _prev_pos)) != std::string::npos) {
    _result.push_back(str.substr(_prev_pos, _pos - _prev_pos));
    _prev_pos = _pos + delimiter.size();
  }
  _result.push_back(str.substr(_prev_pos));
  return _result;
}

std::pair<int, std::string> Config::_trimLeftTab(const std::string& str) {
  int         _tab_count   = 0;
  std::string _trimmed_str = str;

  while (_trimmed_str[0] == '\t') {
    _tab_count++;
    _trimmed_str = _trimmed_str.substr(1);
  }
  return std::make_pair(_tab_count, _trimmed_str);
}

std::string Config::_trimLeftSpace(const std::string& str) {
  std::string _trimmed_str = str;

  while (_trimmed_str[0] == '\t' || _trimmed_str[0] == ' ') {
    _trimmed_str = _trimmed_str.substr(1);
  }
  return _trimmed_str;
}

void Config::_parseRedirection(const std::string& value, LocationInfo& info) {
  std::vector<std::string> _splitted = _split(value, " ");
  if (_splitted.size() != 2) {
    std::cout << "ERROR: invalid config file" << std::endl;
    Log::log()(LOG_LOCATION, "ERROR: invalid config file" + value, ALL);
    std::exit(1);
  }
  try {
    info.redirStatus = std::stoi(_splitted[0]);
  } catch (std::invalid_argument& e) {
    std::cout << "ERROR: invalid config file" << std::endl;
    Log::log()(LOG_LOCATION, "ERROR: invalid config file" + value, ALL);
    std::exit(1);
  }
  info.redirPath = _splitted[1];
}

std::vector<std::string> Config::_parseAllowedMethod(const std::string& value) {
  std::vector<std::string> _allowed_methods;
  std::vector<std::string> _splitted = _split(value, " ");
  for (std::vector<std::string>::iterator it = _splitted.begin(); it != _splitted.end(); ++it) {
    _allowed_methods.push_back(*it);
  }
  return _allowed_methods;
}

std::map<int, std::string> Config::_parseDefaultErrorPage(const std::string& pages) {
  std::map<int, std::string> _result;

  std::vector<std::string> _splitted = _split(pages, " ");
  if (_splitted.size() % 2 != 0) {
    std::cout << "ERROR: invalid config file" << std::endl;
    std::exit(1);
  }
  for (int i = 0; i < _splitted.size(); i += 2) {
    try {
      int _error_code      = std::stoi(_splitted[i]);
      _result[_error_code] = _splitted[i + 1];
    } catch (std::invalid_argument& e) {
      std::cout << "ERROR: invalid config file" << std::endl;
      std::exit(1);
    }
  }
  return _result;
}

// TODO : autoindex 여부 추가
std::stringstream Config::_locatinInfoString(const LocationInfo& info) {
  std::stringstream _ss;
  _ss << "\t"
      << "root: " << info.root << std::endl;
  _ss << "\t"
      << "default_error_page: ";
  for (std::map<int, std::string>::const_iterator it2 = info.defaultErrorPages.begin();
       it2 != info.defaultErrorPages.end(); ++it2) {
    _ss << it2->first << ": " << it2->second << " ";
  }
  _ss << std::endl;
  _ss << "\t"
      << "index: " << info.indexPagePath << std::endl;
  _ss << "\t"
      << "autoindex: " << info.isAutoIndexOn << std::endl;
  _ss << "\t"
      << "cgi_extension: " << info.cgiExtension << std::endl;
  _ss << "\t"
      << "cgi_path: " << info.cgiPath << std::endl;
  _ss << "\t"
      << "redirection: " << info.redirStatus << " " << info.redirPath << std::endl;
  _ss << "\t"
      << "allowed_method: ";
  for (std::vector<std::string>::const_iterator it2 = info.allowedMethods.begin(); it2 != info.allowedMethods.end();
       ++it2) {
    _ss << *it2 << " ";
  }
  _ss << std::endl;
  return _ss;
}

std::stringstream Config::_serverInfoString(const ServerInfo& info) {
  std::stringstream _ss;
  _ss << "----------------------------------------" << std::endl;
  _ss << "server_name: " << info.serverName << std::endl;
  _ss << "root: " << info.root << std::endl;
  _ss << "default_error_page: ";
  for (defaultErrorPagesConstIterator it = info.defaultErrorPages.begin(); it != info.defaultErrorPages.end(); ++it) {
    _ss << it->first << ": " << it->second << " ";
  }
  _ss << std::endl;
  _ss << "client_max_body_size: " << info.maxBodySize << std::endl;
  _ss << "host IP: " << info.hostIp.s_addr << std::endl;
  _ss << "host port: " << info.hostPort << std::endl;
  for (locationInfoConstIterator it = info.locations.begin(); it != info.locations.end(); ++it) {
    _ss << it->first << ": " << std::endl;
    _ss << _locatinInfoString(it->second).str();
  }
  _ss << "----------------------------------------" << std::endl;
  return _ss;
}

void Config::_parsedConfigResult() {
  std::cout << "===============parsed result=====================" << std::endl;
  for (serverInfoConstIterator it = _serverInfos.begin(); it != _serverInfos.end(); ++it) {
    std::cout << _serverInfoString(*it).str();
  }
  std::cout << "=================================================" << std::endl;
}

void Config::_setPorts() {
  for (serverInfoConstIterator it = _serverInfos.begin(); it != _serverInfos.end(); ++it) {
    _ports.push_back(it->hostPort);
  }
  // std::cout << "ports: ";
  // for (std::vector<int>::const_iterator it = _ports.begin(); it != _ports.end(); ++it) {
  //   std::cout << *it << " ";
  // }
}