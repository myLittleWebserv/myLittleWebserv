/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mypark <mypark@student.42seoul.kr>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:30:14 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/24 01:18:54 by mypark           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOG_HPP
#define LOG_HPP

#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class HttpRequest;
class HttpResponse;

#define LOG_LOCATION __FILE__, __LINE__, __func__
#define LOG_DIR "./log_files/"

enum TimestampType { LOG_TITLE, LOG_FILE };
enum LogLocationType { ALL, INFILE, CONSOLE };

const std::string currentTimestamp(TimestampType type = LOG_TITLE);

class Log {
 private:
  std::ofstream _logFile;
  std::string   _failure_message;
  Log();
  ~Log();

 public:
  static Log&    log();
  std::ofstream& getLogStream();
  void           mark(const std::string& mark = "");
  void           mark(bool condition, const std::string& mark = "");
  void           syscall(int ret, const char* file, int line, const char* function, const std::string& success_message,
                         const std::string& failure_message, LogLocationType location = INFILE);
  void           operator()(const std::string& message, LogLocationType location = INFILE);
  void           operator()(const char* file, int line, const char* function, const std::string& message,
                  LogLocationType location = INFILE);
  template <typename ARG>
  void operator()(const std::string& tag, ARG arg, LogLocationType location = INFILE);
  template <typename ARG>
  void operator()(bool condition, const std::string& tag, ARG arg, LogLocationType location = INFILE);

  void printHttpRequest(HttpRequest& request, LogLocationType location = INFILE);
  void printHttpResponse(HttpResponse& response, LogLocationType location = INFILE);
};

template <typename ARG>
void Log::operator()(const std::string& tag, ARG arg, LogLocationType location) {
  std::stringstream logMessage;

  logMessage << tag << " : " << arg;

  if (location == ALL || location == CONSOLE) {
    std::cerr << logMessage.str() << std::endl;
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str() << std::endl;
  }
}

template <typename ARG>
void Log::operator()(bool condition, const std::string& tag, ARG arg, LogLocationType location) {
  if (condition) {
    operator()(tag, arg, location);
  }
}

// enum LogStatus { ALL, INFILE, CONSOLE };

// std::string currentTimeStamp();

// class Log {
//  private:
//   std::ofstream _logFile;

//  public:
//   Log();
//   ~Log();

//   void mark(std::string mark);
//   void operator()(std::string message, LogStatus status = INFILE);
// };

#endif
