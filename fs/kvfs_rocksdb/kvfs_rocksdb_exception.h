/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_exception.hpp
 */

#ifndef KVFS_ROCKSDB_ROCKS_DB_EXCEPTION_HPP
#define KVFS_ROCKSDB_ROCKS_DB_EXCEPTION_HPP

#include <rocksdb/db.h>

namespace kvfs {

class RocksException : public std::exception {
 public:
  RocksException(const rocksdb::Status &status, const std::string &msg);
  RocksException(bool status, const std::string &msg);

  const char *what() const noexcept override;

 private:
  rocksdb::Status status_;
  std::string msg_;
  std::string fullMsg_;
};

}  // namespace kvfs

#endif //KVFS_ROCKSDB_ROCKS_DB_EXCEPTION_HPP
