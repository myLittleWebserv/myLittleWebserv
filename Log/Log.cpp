/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mypark <mypark@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:41:24 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/23 22:30:31 by mypark           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Log.hpp"

#include <cstdlib>
#include <iomanip>

const std::string currentTimestamp(TimestampType type) {
  time_t rawtime;
  time(&rawtime);

  struct tm* timeinfo;
  timeinfo = localtime(&rawtime);

  std::stringstream timestamp;
  if (type == LOG_TITLE) {
    timestamp << std::setfill('0') << std::setw(4) << timeinfo->tm_year + 1900 << std::setw(2) << timeinfo->tm_mon + 1
              << std::setw(2) << timeinfo->tm_mday << "_" << std::setw(2) << timeinfo->tm_hour << std::setw(2)
              << timeinfo->tm_min << std::setw(2) << timeinfo->tm_sec;
  } else {
    timestamp << "[" << std::setfill('0') << std::setw(4) << timeinfo->tm_year + 1900 << "/" << std::setw(2)
              << timeinfo->tm_mon + 1 << "/" << std::setw(2) << timeinfo->tm_mday << " " << std::setw(2)
              << timeinfo->tm_hour << ":" << std::setw(2) << timeinfo->tm_min << ":" << std::setw(2) << timeinfo->tm_sec
              << "] ";
  }

  return timestamp.str();
}

Log::Log() { _logFile.open(currentTimestamp(LOG_TITLE) + ".log", std::ofstream::out | std::ofstream::app); }

Log::~Log() { _logFile.close(); }

Log& Log::log() {
  static Log log;
  return log;
}

std::ofstream& Log::getLogStream() { return _logFile; }

void Log::mark(const std::string& mark) {
  for (int i = 0; i < 30; i++) {
    _logFile << "-";
  }
  _logFile << mark;
  for (int i = 0; i < 30; i++) {
    _logFile << "-";
  }
  _logFile << std::endl;
}

void Log::condition(bool condition, const char* file, int line, const char* function,
                    const std::string& success_message, const std::string& failure_message, LogLocationType location) {
  if (condition && !success_message.empty()) {
    operator()(file, line, function, success_message, location);

  } else {
    operator()(file, line, function, failure_message, location);
    exit(1);
  }
}

void Log::operator()(const char* file, int line, const char* function, const std::string& message,
                     LogLocationType location) {
  std::stringstream logMessage;

  logMessage << currentTimestamp(LOG_FILE) << std::endl
             << "[Logged from : " << file << ":" << function << ":" << line << "] " << std::endl
             << message << std::endl;

  if (location == ALL || location == CONSOLE) {
    std::cout << logMessage.str();
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str();
  }
}
