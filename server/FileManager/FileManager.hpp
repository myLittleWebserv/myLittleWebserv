#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <vector>

#include "Config.hpp"

#define TEMP_REQUEST_PREFIX "temp/request"
#define TEMP_RESPONSE_PREFIX "temp/response"

class FileManager {
  // Types
 public:
  typedef std::vector<int> TempFileFds;

 private:
  static TempFileFds _tempFileFd;
  std::string        _absolutePath;
  std::ifstream      _inFile;
  std::ofstream      _outFile;
  DIR*               _directory;
  dirent*            _entry;
  bool               _isExist;
  bool               _isDirectoy;

  // Method
 private:
  void _updateFileInfo();
  void _appendFileName(std::string file);

  // Constructor & Destructor
 public:
  FileManager() : _directory(NULL), _isExist(false), _isDirectoy(false) {}
  FileManager(const std::string& uri, const LocationInfo& location_info);
  ~FileManager();

  // Interface
 public:
  bool               isFileExist() { return _isExist; }
  bool               isDirectory() { return _isDirectoy; }
  std::ifstream&     inFile() { return _inFile; }
  std::ofstream&     outFile() { return _outFile; }
  const std::string& fileName() { return _absolutePath; }
  int                openFile(const char* file_path, int oflags, mode_t mode);
  void               openInFile() { _inFile.open(_absolutePath.c_str()); }
  void openOutFile(std::ofstream::openmode opt = std::ofstream::out) { _outFile.open(_absolutePath.c_str(), opt); }
  void openDirectoy();
  void removeFile();
  void removeFile(int key_fd);
  void clearTempFd();
  void addIndexToName(const std::string& indexFile);
  std::string readDirectoryEntry();
};

#endif