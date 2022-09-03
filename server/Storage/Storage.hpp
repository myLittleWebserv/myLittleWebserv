// #ifndef STORAGE_HPP
#pragma once

#include <string>
#include <vector>

#include "Log.hpp"
#define READ_BUFFER_SIZE 8192

class Storage : private std::vector<unsigned char> {
  // Types;
 public:
  typedef std::vector<unsigned char>           vector;
  typedef std::vector<unsigned char>::iterator iterator;

  // Member Variable
 protected:
  unsigned char _buffer[READ_BUFFER_SIZE];
  int           _pos;

  // Constructor
 public:
  Storage() : _pos(0) {}
  virtual ~Storage() {}

  // Interface
 public:
  using vector::begin;
  using vector::clear;
  using vector::data;
  using vector::end;
  using vector::insert;
  using vector::size;
  void            movePos(int move) { _pos += move; }
  int             remains() { return static_cast<int>(size()) - _pos; }
  bool            empty() const { return static_cast<int>(size()) == _pos; }
  vector::pointer currentPos() { return data() + _pos; }
  std::string     getLine();
};

// #endif
