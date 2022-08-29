#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <dirent.h>
#include <sys/stat.h>

#include <fstream>

#include "Config.hpp"

class FileManager {
 private:
  std::string   _fileName;
  std::ifstream _inFile;
  std::ofstream _outFile;
  DIR*          _directory;
  dirent*       _entry;
  bool          _isExist;
  bool          _isDirectoy;

  // Method
 private:
  void _updateFileInfo();
  void _appendToFileName(std::string file);

 public:
  FileManager(const std::string& uri, const LocationInfo& location_info);
  ~FileManager();
  bool               isFileExist() { return _isExist; }
  bool               isDirectory() { return _isDirectoy; }
  std::ifstream&     inFile() { return _inFile; }
  std::ofstream&     outFile() { return _outFile; }
  const std::string& fileName() { return _fileName; }
  void               openInFile() { _inFile.open(_fileName.c_str()); }
  void        openOutFile(std::ofstream::openmode opt = std::ofstream::out) { _outFile.open(_fileName.c_str(), opt); }
  void        openDirectoy();
  void        removeFile();
  void        addIndexToName(const std::string& indexFile);
  std::string readDirectoryEntry();
};

#endif