/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:41:24 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/18 19:16:22 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Log.hpp"

Log::Log() {
}

Log::~Log() {
}

Log& Log::getInstance() {
  static Log instance;
  
  return instance;
}

void Log::operator()(std::string fileName, std::string methodName, int lineNum, std::string msg, int errno, LogStatus status) {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
  std::ofstream _logfile = std::ofstream("log.txt");
  
  if (status == ALL || status == INFILE) {
    _logfile << "logged time: " << std::ctime(&currentTime) << std::endl;
    _logfile << fileName << ":" << methodName << ":" << lineNum << ":" << msg << ":" << errno << std::endl;
  } else if (status == ALL || status == CONSOLE) {
    std::cout << "logged time: " << std::ctime(&currentTime) << std::endl;
    std::cout << fileName << ":" << methodName << ":" << lineNum << ":" << msg << ":" << errno << std::endl;
  }
}