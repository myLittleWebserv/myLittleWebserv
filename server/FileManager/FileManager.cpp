#include "FileManager.hpp"

#include "Router.hpp"

FileManager::TempFileFds FileManager::_tempFileFd;

FileManager::~FileManager() {
  if (_directory)
    closedir(_directory);
}

FileManager::FileManager(const std::string& uri, const LocationInfo& location_info)
    : _absolutePath(location_info.root), _directory(NULL), _isExist(false), _isDirectoy(false) {
  std::string file_name = uri.substr(location_info.id.size());
  _appendFileName(file_name);

  Log::log()(true, "File path", _absolutePath);
  Log::log()(true, "File name", file_name);

  _updateFileInfo();
}

void FileManager::addIndexToName(const std::string& indexFile) {
  _appendFileName(indexFile);

  Log::log()(true, "File path", _absolutePath);

  _updateFileInfo();
}

void FileManager::openDirectoy() { _directory = opendir(_absolutePath.c_str()); }

std::string FileManager::readDirectoryEntry() {
  _entry = readdir(_directory);
  if (_entry == NULL) {
    return "";
  }
  return std::string(_entry->d_name);
}

void FileManager::removeFile() {
  if (!_isDirectoy) {
    std::remove(_absolutePath.c_str());
  } else {
    openDirectoy();
    std::string file_name = readDirectoryEntry();
    while (!file_name.empty()) {
      if (file_name != "." && file_name != "..") {
        _appendFileName(file_name);
        _updateFileInfo();
        removeFile();
      }
      file_name = readDirectoryEntry();
    }
  }
}

int FileManager::openFile(const char* file_path, int oflag, mode_t mode = 0644) {
  int fd = open(file_path, oflag, mode);
  _tempFileFd.push_back(fd);
  return fd;
}

void FileManager::clearTempFileFd() {
  for (TempFileFds::iterator it = _tempFileFd.begin(); it != _tempFileFd.end(); ++it) {
    close(*it);
  }
  if (_tempFileFd.size() > 0) {
    Log::log()(LOG_LOCATION, "(DONE) fds of temporary file closed", INFILE);
  }
  _tempFileFd.clear();
}

void FileManager::removeFile(int key_fd) {
  std::stringstream key;
  key << key_fd;
  std::string temp_request  = TEMP_REQUEST_PREFIX + key.str();
  std::string temp_response = TEMP_RESPONSE_PREFIX + key.str();

  if (unlink(temp_request.c_str()) == -1 || unlink(temp_response.c_str()) == -1) {
    throw Router::ServerSystemCallException();
  }
  Log::log()(LOG_LOCATION, "(DONE) temporary file removed", INFILE);
}

void FileManager::_appendFileName(std::string back) {
  if (back.c_str()[0] != '/' && *_absolutePath.rbegin() != '/') {
    _absolutePath += "/";
  } else if (back.c_str()[0] == '/' && *_absolutePath.rbegin() == '/') {
    back = back.substr(1);
  }
  _absolutePath += back;
}

void FileManager::_updateFileInfo() {
  _isExist    = false;
  _isDirectoy = false;

  struct stat buf;
  int         ret = lstat(_absolutePath.c_str(), &buf);

  if (ret == -1) {
    return;
  }

  _isExist    = true;
  _isDirectoy = S_ISDIR(buf.st_mode);
}
