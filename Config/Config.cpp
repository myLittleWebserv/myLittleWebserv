/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/20 13:37:55 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/20 14:25:14 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

Config::Config(const std::string& confFile) {
  _readConfigFile(confFile);
  for (int i = 0; )
}

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
