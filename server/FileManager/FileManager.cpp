#include "FileManager.hpp"

#include "syscall.hpp"

FileManager::FdSet FileManager::_fileFdSet;

FileManager::~FileManager() {
  if (_directory)
    closedir(_directory);
}

FileManager::FileManager(const std::string& uri, const LocationInfo& location_info)
    : _absolutePath(location_info.root), _directory(NULL), _isExist(false), _isDirectoy(false), _isConflict(false) {
  std::string file_name = uri.substr(location_info.id.size());
  if (!file_name.empty())
    _appendFileName(file_name);

  // Log::log()(true, "File path", _absolutePath);
  // Log::log()(true, "File name", file_name);

  _updateFileInfo();
}

void FileManager::appendToPath(const std::string& indexFile) {
  _appendFileName(indexFile);

  // Log::log()(true, "File path", _absolutePath);

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

void FileManager::clearFileFds() {
  for (FdSet::iterator it = _fileFdSet.begin(); it != _fileFdSet.end(); ++it) {
    ft::syscall::close(*it);
  }
  if (_fileFdSet.size() > 0) {
    Log::log()(LOG_LOCATION, "(DONE) fds of temporary file closed", INFILE);
  }
  _fileFdSet.clear();
}

void FileManager::removeTempFileByKey(int key_fd) {
  std::stringstream key;
  key << key_fd;
  std::string temp_request  = TEMP_REQUEST_PREFIX + key.str();
  std::string temp_response = TEMP_RESPONSE_PREFIX + key.str();

  ft::syscall::unlink(temp_request.c_str());
  ft::syscall::unlink(temp_response.c_str());
  Log::log()(LOG_LOCATION, "(DONE) temporary file removed", INFILE);
}

void FileManager::removeFile(const std::string& file_name) {
  ft::syscall::unlink(file_name.c_str());
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

bool FileManager::_isDirExist(const std::string& file_path) {
  struct stat buf;
  int         ret = ::lstat(file_path.c_str(), &buf);

  if (ret == -1) {
    return false;
  }

  if (S_ISDIR(buf.st_mode)) {
    return true;
  }
  _isConflict = true;
  return false;
}

void FileManager::registerFileFdToClose(int fd) {
  if (fd != -1) {
    _fileFdSet.insert(fd);
  }
  Log::log()(fd != -1, "registered fd", fd);
}

bool FileManager::isConflict() {
  _isConflict       = false;
  size_t      delim = _absolutePath.find('/');
  std::string path  = _absolutePath.substr(0, delim);

  while (1) {
    // Log::log()(true, "path.isConflict", path);
    if (_isDirExist(path)) {
      ;
    } else if (_isConflict) {
      break;
    } else {
      mkdir(path.c_str(), 0777);
    }
    delim = _absolutePath.find('/', delim + 1);
    if (delim == std::string::npos)
      break;
    path = _absolutePath.substr(0, delim);
  }

  return _isConflict;
}