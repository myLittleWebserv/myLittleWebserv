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

 public:
  FileManager(const std::string& uri, const LocationInfo& location_info);
  ~FileManager();
  bool               isFileExist() { return _isExist; }
  bool               isDirectory() { return _isDirectoy; }
  std::ifstream&     inFile() { return _inFile; }
  std::ofstream&     outFile() { return _outFile; }
  const std::string& fileName() { return _fileName; }
  void               openInfile() { _inFile.open(_fileName.c_str()); }
  void        openOutfile(std::ofstream::openmode opt = std::ofstream::out) { _outFile.open(_fileName.c_str(), opt); }
  void        openDirectoy();
  void        addIndexToName(const std::string& indexFile);
  std::string readDirectoryEntry();
};

FileManager::~FileManager() {
  if (_directory)
    closedir(_directory);
}

FileManager::FileManager(const std::string& uri, const LocationInfo& location_info)
    : _fileName(location_info.root), _directory(NULL), _isExist(false), _isDirectoy(false) {
  std::string file_pos = uri.substr(location_info.id.size());
  if (file_pos.c_str()[0] != '/' && *_fileName.rbegin() != '/') {
    _fileName += "/";
  } else if (file_pos.c_str()[0] == '/' && *_fileName.rbegin() == '/') {
    file_pos = file_pos.substr(1);
  }
  _fileName += file_pos;

  Log::log()(true, "File name", _fileName);
  Log::log()(true, "File pos", file_pos);

  _updateFileInfo();
}

void FileManager::addIndexToName(const std::string& indexFile) {
  if (*_fileName.rbegin() != '/') {
    _fileName += "/";
  }
  _fileName += indexFile;
  _updateFileInfo();
}

void FileManager::openDirectoy() { opendir(_fileName.c_str()); }

std::string FileManager::readDirectoryEntry() {
  _entry = readdir(_directory);
  if (_entry == NULL) {
    return "";
  }
  return std::string(_entry->d_name);
}

void FileManager::_updateFileInfo() {
  _isExist    = false;
  _isDirectoy = false;

  struct stat buf;
  int         ret = lstat(_fileName.c_str(), &buf);

  if (ret == -1) {
    return;
  }

  _isExist    = true;
  _isDirectoy = S_ISDIR(buf.st_mode);
}

#endif