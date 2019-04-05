#include <utility>

/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_exception.cpp
 */

#include "kvfs_rocksdb_exception.h"

namespace kvfs {
RocksException::RocksException(
    rocksdb::Status status,
    std::string msg)
    : status_(std::move(status)), msg_(std::move(msg)) {
  fullMsg_ = "[FS CRITICAL ERROR] : ";
  fullMsg_ += "(Store Status: " + status_.ToString() + ")";
  fullMsg_ += "\n\t\t (" + msg_ + ")";
}
const char *RocksException::what() const noexcept {
  return fullMsg_.c_str();
}
RocksException::RocksException(const bool status, std::string msg)
    : msg_(std::move(msg)) {
  std::string status_str = status ? "true" : "false";
  fullMsg_ = "[FS CRITICAL ERROR] : ";
  fullMsg_ += "(Store Status: " + status_str + ")";
  fullMsg_ += "\n\t\t (" + msg_ + ")";
}
}  // namespace kvfs