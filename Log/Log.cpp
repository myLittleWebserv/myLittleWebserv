/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:41:24 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/18 18:44:47 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Log.hpp"

Log::Log() {
  _logfile.open("log.txt", std::ios::app);
}

Log::~Log() {
  _logfile.close();
}

Log& Log::getInstance() {
  static Log instance;
  
  return instance;
}

void Log::operator()(std::string fileName, std::string methodName, std::string lineNum
                    , std::string msg, int errno, LogStatus status) {
  if (status == ALL || status == FILE) {
    _logfile << fileName << ":" << methodName << ":" << lineNum << ":" << msg << ":" << errno << std::endl;
  } else if (status == CONSOLE) {
    std::cout << fileName << ":" << methodName << ":" << lineNum << ":" << msg << ":" << errno << std::endl;
  }
}