#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <set>

#include "Config.hpp"

#define TEMP_REQUEST_PREFIX "temp/request"
#define TEMP_RESPONSE_PREFIX "temp/response"
#define TEMP_TRASH "temp/trash"

class FileManager {
  // Types
 public:
  typedef std::set<int> TempFileFds;

  // Static
 private:
  static TempFileFds _tempFileFd;

 public:
  static void clearTempFileFd();
  static void registerFileFdToClose(int fd) { _tempFileFd.insert(fd); }
  static void removeTempFileByKey(int key_fd);
  static void removeFile(const std::string& file_name);

  // Member Variable
 private:
  std::string   _absolutePath;
  std::ifstream _inFile;
  std::ofstream _outFile;
  DIR*          _directory;
  dirent*       _entry;
  bool          _isExist;
  bool          _isDirectoy;

  // Method
 private:
  void _updateFileInfo();
  void _appendFileName(std::string file);

  // Constructor & Destructor
 public:
  FileManager() : _directory(NULL), _isExist(false), _isDirectoy(false) {}
  FileManager(const std::string& file_path)
      : _absolutePath(file_path), _directory(NULL), _isExist(false), _isDirectoy(false) {}
  FileManager(const std::string& uri, const LocationInfo& location_info);
  ~FileManager();

  // Interface
 public:
  bool               isFileExist() { return _isExist; }
  bool               isDirectory() { return _isDirectoy; }
  const std::string& filePath() { return _absolutePath; }
  void               openDirectoy();
  void               removeFile();
  void               appendToPath(const std::string& indexFile);
  std::string        readDirectoryEntry();
};

#endif