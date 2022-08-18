/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Log.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaemjung <jaemjung@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/08/18 18:30:14 by jaemjung          #+#    #+#             */
/*   Updated: 2022/08/18 19:14:32 by jaemjung         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOG_HPP
# define LOG_HPP

# include <iostream>
# include <string>
# include <fstream>
# include <chrono>

enum LogStatus {
  ALL,
  INFILE,
  CONSOLE
};

class Log {
  private:
    Log();
    Log(Log const & ref);
    Log& operator=(Log const & ref);
    ~Log();

  public:
    static Log& getInstance();
    void operator()(std::string fileName, std::string methodName, int lineNum
                    , std::string msg, int errno = 0, LogStatus status = ALL);
};

#endif