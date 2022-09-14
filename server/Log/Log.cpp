#include "Log.hpp"

#include <cstdlib>
#include <iomanip>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

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

Log::Log() : processedConnection(0) {
  _logFile.open((LOG_DIR + currentTimestamp(LOG_TITLE) + ".log").c_str(), std::ofstream::out | std::ofstream::app);
}

Log::~Log() { _logFile.close(); }

Log& Log::log() {
  static Log log;
  return log;
}

void Log::printStatus() { operator()(true, "Processed Connection", processedConnection, ALL); }

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

void Log::mark(bool condition, const std::string& m) {
  if (condition) {
    mark(m);
    throw _failure_message;
  }
}

void Log::syscall(int ret, const char* file, int line, const char* function, const std::string& success_message,
                  const std::string& failure_message, LogLocationType location) {
  if (ret == -1) {
    operator()(file, line, function, failure_message, location);
    operator()("errno", strerror(errno));
    _failure_message = failure_message;
  } else if (!success_message.empty()) {
    operator()(file, line, function, success_message, location);
  }
}

void Log::operator()(const std::string& message, LogLocationType location) {
  std::stringstream logMessage;

  logMessage << message;

  if (location == ALL || location == CONSOLE) {
    std::cerr << logMessage.str() << std::endl;
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str() << std::endl;
  }
}

void Log::operator()(const char* file, int line, const char* function, const std::string& message,
                     LogLocationType location) {
  std::stringstream logMessage;

  logMessage << currentTimestamp(LOG_FILE) << '\n'
             << "[Logged from : " << file << ":" << function << ":" << line << "] " << std::endl
             << message;

  if (location == ALL || location == CONSOLE) {
    std::cerr << logMessage.str() << std::endl;
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str() << std::endl;
  }
}

void Log::printHttpRequest(HttpRequest& request, LogLocationType location) {
  std::stringstream logMessage;

  logMessage << "HttpRequest:"
             << "\nmethod: " << request.method() << "\nuri: " << request.uri()
             << "\nhttpVersion: " << request.httpVersion() << "\ncontentLength: " << request.contentLength()
             << "\ncontentType: " << request.contentType() << "\nhostPort: " << request.hostPort()
             << "\nhostName: " << request.hostName() << '\n';

  if (location == ALL || location == CONSOLE) {
    std::cerr << logMessage.str() << std::endl;
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str() << std::endl;
  }
}

void Log::printHttpResponse(HttpResponse& response, LogLocationType location) {
  std::stringstream logMessage;

  logMessage << "HttpResponse:\n";

  (void)response;
  // logMessage << response.header();
  // for (int i = 0; i < response.contentLength(); ++i) {
  //   logMessage << response.body()[i];
  // }
  if (response._storage.size() < 1000000) {
    for (Storage::vector::pointer it = response._storage.currentReadPos(); it != response._storage.currentWritePos();
         ++it) {
      logMessage << *it;
    }
  }

  if (location == ALL || location == CONSOLE) {
    std::cerr << logMessage.str() << std::endl;
  }
  if (location == ALL || location == INFILE) {
    _logFile << logMessage.str() << std::endl;
  }
}
