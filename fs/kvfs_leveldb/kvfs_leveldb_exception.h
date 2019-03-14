/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_exception.h
 */

#ifndef KVFS_KVFS_LEVELDB_EXCEPTION_H
#define KVFS_KVFS_LEVELDB_EXCEPTION_H

#include <exception>
#include <leveldb/status.h>

namespace kvfs {

class LevelDBException : public std::exception {
 public:
  LevelDBException(const leveldb::Status &status, const std::string &msg);
  LevelDBException(bool status, const std::string &msg);

  const char *what() const noexcept override;

 private:
  leveldb::Status status_;
  std::string msg_;
  std::string fullMsg_;
};

}  // namespace kvfs

#endif //KVFS_KVFS_LEVELDB_EXCEPTION_H
