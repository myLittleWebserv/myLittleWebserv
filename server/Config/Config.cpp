#include "Config.hpp"

#include <cstdlib>
#include <sstream>

Config::Config(const std::string& confFile) {
  _readConfigFile(confFile);
  _startParse();
  _setPorts();
  _parsedConfigResult();
}

Config::~Config() {}

void Config::_readConfigFile(const std::string& confFile) {
  std::ifstream _file(confFile.c_str());

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
      ServerInfo serverInfo = _parseServer(++it, _configContent.end());
      _serverInfos.push_back(serverInfo);
    } else {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file", ALL);
      std::exit(1);
    }
  }
  if (_serverInfos.empty()) {
    _serverInfos.push_back(_init_serverInfo());
    _serverInfos.back().locations["/"] = _init_locationInfo(_serverInfos.back());
  }
}

ServerInfo Config::_parseServer(configIterator& it, const configIterator& end) {
  ServerInfo _server_info = _init_serverInfo();

  while (it != end && *it != "server" && *it != "\n") {
    std::pair<int, std::string> _trimmed = _trimLeftTab(*it);
    // if (_trimmed.first != 1) {
    //   break;
    // }
    std::vector<std::string> _splitted = _split(_trimmed.second, ":");
    if (_splitted.size() != 2) {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file");
      std::exit(1);
    }
    std::string       _identifier = _splitted[0];
    std::stringstream _value(_trimLeftSpace(_splitted[1]));
    int               vi;
    if (_identifier == "client_max_body_size") {
      _value >> vi;
      _server_info.maxBodySize = vi;
    } else if (_identifier == "root") {
      _server_info.root = _value.str();
    } else if (_identifier == "default_error_page") {
      _parseDefaultErrorPage(_value.str(), _server_info.defaultErrorPages);
    } else if (_identifier == "host") {
      inet_aton(_value.str().c_str(), &_server_info.hostIp);  // TODO: ip 주소 잘못됐을 때 에러처리
    } else if (_identifier == "port") {
      _value >> vi;
      _server_info.hostPort = vi;  // TODO: 포트넘버 범위 벗어나거나 잘못됐을 때 에러처리
    } else if (_identifier == "server_name") {
      _server_info.serverName = _value.str();
    } else if (_identifier == "location") {
      _server_info.locations[_value.str()] = _parseLocation(++it, _server_info, _value.str());
      continue;
    } else {
      std::cout << "ERROR: invalid config file" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid config file" + _identifier + _value.str(), ALL);
      std::exit(1);
    }
    ++it;
  }
  if (_server_info.locations.empty()) {
    _server_info.locations["/"] = _init_locationInfo(_server_info);
  }
  return _server_info;
}

LocationInfo Config::_parseLocation(configIterator& it, const ServerInfo& serverInfo, const std::string& id) {
  LocationInfo _location_info = _init_locationInfo(serverInfo);
  _location_info.id           = id;
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
  std::stringstream value_stream(value);
  int               vi;
  if (identifier == "client_max_body_size") {
    value_stream >> vi;
    info.maxBodySize = vi;
  } else if (identifier == "root") {
    info.root = value_stream.str();
  } else if (identifier == "default_error_page") {
    _parseDefaultErrorPage(value_stream.str(), info.defaultErrorPages);
  } else if (identifier == "allowed_method") {
    info.allowedMethods = _parseAllowedMethod(value_stream.str());
  } else if (identifier == "cgi_extension") {
    info.cgiExtension = value_stream.str();
  } else if (identifier == "cgi_path") {
    info.cgiPath = value_stream.str();
  } else if (identifier == "redirection") {
    _parseRedirection(value_stream.str(), info);
  } else if (identifier == "index") {
    info.indexPagePath = value_stream.str();
  } else {
    std::cout << "ERROR: invalid config file" << std::endl;
    Log::log()(LOG_LOCATION, "ERROR: invalid config file" + identifier + value_stream.str(), ALL);
    std::exit(1);
  }
}

ServerInfo Config::_init_serverInfo() {
  ServerInfo _server_info;
  _server_info.maxBodySize       = DEFAULT_MAX_BODY_SIZE;
  _server_info.root              = "";
  _server_info.defaultErrorPages = _init_defaultErrorPages();
  _server_info.hostIp.s_addr     = INADDR_ANY;
  _server_info.hostPort          = HTTP_DEFAULT_PORT;
  _server_info.serverName        = "";
  return _server_info;
}

LocationInfo Config::_init_locationInfo(const ServerInfo& serverInfo) {
  LocationInfo _location_info;

  _location_info.id                = "/";
  _location_info.maxBodySize       = serverInfo.maxBodySize;
  _location_info.root              = serverInfo.root;
  _location_info.defaultErrorPages = serverInfo.defaultErrorPages;
  _location_info.allowedMethods    = _defaultAllowedMethods();
  _location_info.cgiExtension      = "";
  _location_info.cgiPath           = "";
  _location_info.indexPagePath     = "";
  _location_info.isAutoIndexOn     = false;
  _location_info.redirStatus       = -1;
  _location_info.redirPath         = "index";
  _location_info.hostIp            = serverInfo.hostIp;
  _location_info.hostPort          = serverInfo.hostPort;
  _location_info.serverName        = serverInfo.serverName;
  _location_info.redirStatus       = -1;
  return _location_info;
}

std::map<int, std::string> Config::_init_defaultErrorPages() {
  std::map<int, std::string> _default_error_pages;
  for (int i = 0; i < ERROR_PAGES_COUNT; i++) {
    _default_error_pages[_error_codes[i]] = ERROR_PAGES_PATH + _itoa(_error_codes[i]) + ".html";
  }
  return _default_error_pages;
}

std::vector<std::string> Config::_defaultAllowedMethods() {
  std::vector<std::string> _allowed_methods;
  _allowed_methods.push_back("GET");
  _allowed_methods.push_back("POST");
  _allowed_methods.push_back("HEAD");
  _allowed_methods.push_back("PUT");
  _allowed_methods.push_back("DELETE");
  return _allowed_methods;
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
    std::stringstream ss(_splitted[0]);
    int               si;
    ss >> si;
    info.redirStatus = si;
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
    if (*it == "GET" || *it == "POST" || *it == "PUT" || *it == "DELETE" || *it == "HEAD") {
      _allowed_methods.push_back(*it);
    } else {
      std::cout << "ERROR: invalid HTTP Method" << std::endl;
      Log::log()(LOG_LOCATION, "ERROR: invalid HTTP Method" + value, ALL);
      std::exit(1);
    }
  }
  return _allowed_methods;
}

void Config::_parseDefaultErrorPage(const std::string& pages, std::map<int, std::string>& defaultErrorPages) {
  std::vector<std::string> _splitted = _split(pages, " ");
  if (_splitted.size() % 2 != 0) {
    std::cout << "ERROR: invalid config file" << std::endl;
    std::exit(1);
  }
  for (std::vector<std::string>::size_type i = 0; i < _splitted.size(); i += 2) {
    try {
      std::stringstream ss(_splitted[i]);
      int               si;
      ss >> si;
      int _error_code      = si;
      defaultErrorPages[_error_code] = _splitted[i + 1];
    } catch (std::invalid_argument& e) {
      std::cout << "ERROR: invalid config file" << std::endl;
      std::exit(1);
    }
  }
}

void Config::_locatinInfoString(std::stringstream& _ss, const LocationInfo& info) {
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
}

void Config::_serverInfoString(std::stringstream& _ss, const ServerInfo& info) {
  _ss << "----------------------------------------" << std::endl;
  _ss << "server_name: " << info.serverName << std::endl;
  _ss << "root: " << info.root << std::endl;
  _ss << "default_error_page: ";
  for (defaultErrorPagesConstIterator it = info.defaultErrorPages.begin(); it != info.defaultErrorPages.end(); ++it) {
    _ss << it->first << ": " << it->second << " ";
  }
  _ss << std::endl;
  in_addr addr = info.hostIp;
  _ss << "client_max_body_size: " << info.maxBodySize << std::endl;
  _ss << "host IP: " << inet_ntoa(addr) << std::endl;
  _ss << "host port: " << info.hostPort << std::endl;
  for (locationInfoConstIterator it = info.locations.begin(); it != info.locations.end(); ++it) {
    _ss << it->first << ": " << std::endl;
    _locatinInfoString(_ss, it->second);
  }
  _ss << "----------------------------------------" << std::endl;
}

void Config::_parsedConfigResult() {
  std::cout << "===============parsed result=====================" << std::endl;
  for (serverInfoConstIterator it = _serverInfos.begin(); it != _serverInfos.end(); ++it) {
    std::stringstream _ss;
    _serverInfoString(_ss, *it);
    std::cout << _ss.str();
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

std::string Config::_itoa(int i) {
  std::stringstream ss;
  ss << i;
  return ss.str();
}

int Config::_error_codes[ERROR_PAGES_COUNT] = {400, 402, 404, 405, 413, 500, 501, 502, 503, 504, 505};

std::vector<int>&        Config::getPorts() { return _ports; }
std::vector<ServerInfo>& Config::getServerInfos() { return _serverInfos; }
