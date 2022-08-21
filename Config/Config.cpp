/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/20 13:37:55 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/22 00:42:34 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

Config::Config(const std::string& confFile) {
  _readConfigFile(confFile);
  _startParse();
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

// TODO : 서버가 여러 개 있을 때 리스트에 담기도록 잘 하기!
void Config::_startParse() {
  configIterator it = _configContent.begin();
  while (it != _configContent.end()) {
    if (*it == "server") {
      ServerInfo serverInfo = _parseServer(++it);
      _serverInfos[serverInfo.serverName] = serverInfo;
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
    std::cout << "current token: " << *it << std::endl;
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
    std::cout << _identifier << " " << _value << std::endl;
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
  std::cout << "--------------parse Location start-------------" << std::endl;
  while (*it != "\n") {
    std::cout << "current token: " << *it << std::endl;
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
      std::cout << "ERROR: invalid config file"
                << "current token: " << _trimmed.second << std::endl;
      std::exit(1);
    }
    std::string _identifier = _splitted[0];
    std::string _value      = _trimLeftSpace(_splitted[1]);
    std::cout << _identifier << " " << _value << std::endl;
    if (_identifier == "client_max_body_size") {
      _location_info.maxBodySize = std::stoi(_value);
    } else if (_identifier == "root") {
      _location_info.root = _value;
    } else if (_identifier == "default_error_page") {
      _location_info.defaultErrorPages = _parseDefaultErrorPage(_value);
    } else if (_identifier == "allowed_method") {
      _location_info.allowedMethods = _parseAllowedMethod(_value);
    } else if (_identifier == "cgi_extension") {
      _location_info.cgiExtension = _value;
    } else if (_identifier == "cgi_path") {
      _location_info.cgiPath = _value;
    } else if (_identifier == "redirection") {
      _parseRedirection(_value, _location_info);
    } else if (_identifier == "index") {
      _location_info.indexPagePath = _value;
    } else {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file" + _identifier + _value, ALL);
      std::exit(1);
    }
    it++;
  }
  return _location_info;
}

LocationInfo Config::_init_locationInfo(const ServerInfo& serverInfo) {
  LocationInfo _location_info;
  _location_info.isAutoIndexOn     = false;
  _location_info.root              = serverInfo.root;
  _location_info.defaultErrorPages = serverInfo.defaultErrorPages;
  _location_info.maxBodySize       = serverInfo.maxBodySize;
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