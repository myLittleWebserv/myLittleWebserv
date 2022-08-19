/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42seoul.kr>     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:30:14 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/20 00:27:48 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOG_HPP
#define LOG_HPP

#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

enum TimestampType { LOG_TITLE, LOG_FILE };
enum LogLocationType { ALL, INFILE, CONSOLE };

const std::string currentTimestamp(TimestampType type = LOG_TITLE);

class Log {
 private:
  std::ofstream _logFile;

 public:
  Log();
  ~Log();

  std::ofstream& getLogStream();
  void           mark(const std::string& mark = "");
  void           operator()(const char* file, int line, const char* function, const std::string& message, LogLocationType location = INFILE);
};

static Log log;

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