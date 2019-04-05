/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_exception.cpp
 */

#include "kvfs_leveldb_exception.h"

kvfs::LevelDBException::LevelDBException(const leveldb::Status &status, const std::string &msg)
    : status_(status), msg_(msg) {
  fullMsg_ = "[FS CRITICAL ERROR] : ";
  fullMsg_ += "(KVStore Status: " + status_.ToString() + ")";
  fullMsg_ += "\n\t\t (" + msg_ + ")";
}
kvfs::LevelDBException::LevelDBException(bool status, const std::string &msg) : msg_(msg) {
  std::string status_str = status ? "true" : "false";
  fullMsg_ = "[FS CRITICAL ERROR] : ";
  fullMsg_ += "(KVStore Status: " + status_str + ")";
  fullMsg_ += "\n\t\t (" + msg_ + ")";
}

const char *kvfs::LevelDBException::what() const noexcept {
  return fullMsg_.c_str();
}