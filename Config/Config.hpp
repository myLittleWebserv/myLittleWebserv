/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:06:58 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/18 18:24:15 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

# include <vector>
# include <map>
# include <string>

typedef struct LocationInfo {
  int                         maxBodySize;
  std::string                 root;
  std::map<int, std::string>  defaultErrorPages;
  std::vector<std::string>    allowedMethods;
  std::string                 cgiExtension;
  std::string                 cgiPath;
  std::string                 indexPagePath;
  bool                        isAutoIndexOn;
  int                         redirStatus;
  std::string                 redirPath;
};

typedef struct ServerInfo {
  int                                 maxBodySize;
  std::string                         root;
  std::map<int, std::string>          defaultErrorPages;
  unsigned int                        hostIp;
  int                                 hostPort;
  std::string                         serverName;
  std::map<std::string, LocationInfo> locations;
};

class Config {
  
  private:
    std::vector<ServerInfo> _serverInfos;
    std::vector<int>        _ports;
    
  public:
  // Constructor
    Config();
    virtual ~Config();
    std::vector<ServerInfo> getServerInfos();
    std::vector<int>        getPorts();
  
};

#endif