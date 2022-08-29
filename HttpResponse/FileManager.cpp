#include "FileManager.hpp"

FileManager::~FileManager() {
  if (_directory)
    closedir(_directory);
}

FileManager::FileManager(const std::string& uri, const LocationInfo& location_info)
    : _fileName(location_info.root), _directory(NULL), _isExist(false), _isDirectoy(false) {
  std::string file_pos = uri.substr(location_info.id.size());
  _appendToFileName(file_pos);

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

void FileManager::removeFile() {
  if (!_isDirectoy) {
    std::remove(_fileName.c_str());
  } else {
    openDirectoy();
    std::string file_name = readDirectoryEntry();
    while (!file_name.empty()) {
      if (file_name != "." && file_name != "..") {
        _appendToFileName(file_name);
        _updateFileInfo();
        removeFile();
      }
      file_name = readDirectoryEntry();
    }
  }
}

void FileManager::_appendToFileName(std::string back) {
  if (back.c_str()[0] != '/' && *_fileName.rbegin() != '/') {
    _fileName += "/";
  } else if (back.c_str()[0] == '/' && *_fileName.rbegin() == '/') {
    back = back.substr(1);
  }
  _fileName += back;
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
