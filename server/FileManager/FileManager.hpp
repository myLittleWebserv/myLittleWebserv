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
  typedef std::set<int> FdSet;

  // Static
 private:
  static FdSet _fileFdSet;

 public:
  static void clearFileFds();
  static void registerFileFdToClose(int fd);
  static void removeTempFileByKey(int key_fd);
  static void removeFile(const std::string& file_name);

  // Member Variable
 private:
  std::string _absolutePath;
  DIR*        _directory;
  dirent*     _entry;
  bool        _isExist;
  bool        _isDirectoy;
  bool        _isConflict;

  // Method
 private:
  void _updateFileInfo();
  void _appendFileName(std::string file);
  bool _isDirExist(const std::string& file_path);
  // Constructor & Destructor
 public:
  FileManager() : _directory(NULL), _isExist(false), _isDirectoy(false) {}
  FileManager(const std::string& file_path)
      : _absolutePath(file_path), _directory(NULL), _isExist(false), _isDirectoy(false) {}
  FileManager(const std::string& uri, const LocationInfo& location_info);
  ~FileManager();

  // Interface
 public:
  bool               isConflict();
  bool               isFileExist() { return _isExist; }
  bool               isDirectory() { return _isDirectoy; }
  bool               isConflict();
  const std::string& filePath() { return _absolutePath; }
  void               openDirectoy();
  void               removeFile();
  void               appendToPath(const std::string& indexFile);
  std::string        readDirectoryEntry();
};

#endif