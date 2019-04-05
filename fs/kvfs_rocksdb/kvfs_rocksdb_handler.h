/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_handler.hpp
 */

#ifndef KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP
#define KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP

#include <memory>
#include <string>
#include <kvfs_config.h>
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/cache.h"

namespace kvfs {

/**
 * This struct is the holder of the database
 * required to interact with our local rocksdb store.
 */
struct RocksHandles {
  std::unique_ptr<rocksdb::DB> db;

  ~RocksHandles();

  /**
   * Returns an instance of a RocksDB that uses the specified directory for
   * storage. If there is an existing RocksDB at that path with
   * then it will be opened and
   * returned.  If there is no existing RocksDB at that location a new one will
   * be initialized .
   */
  explicit RocksHandles(const std::string &dbPath);

  RocksHandles(const RocksHandles &) = delete;
  RocksHandles &operator=(const RocksHandles &) = delete;
  RocksHandles(RocksHandles &&) = default;
  RocksHandles &operator=(RocksHandles &&) = default;
};

/**
 * @brief Holder for iterator
 */
struct RocksIterator {
  std::unique_ptr<rocksdb::Iterator> it;

  ~RocksIterator();

  explicit RocksIterator(RocksHandles db);

  RocksIterator(const RocksIterator &) = delete;

  RocksIterator &operator=(const RocksIterator &) = delete;

  RocksIterator(RocksIterator &&) = default;

  RocksIterator &operator=(RocksIterator &&) = default;
};

struct RocksCache {
  std::shared_ptr<rocksdb::Cache> cache;

  ~RocksCache();

  RocksCache();

  size_t get_size();

  RocksCache(const RocksCache &) = default;

  RocksCache &operator=(const RocksCache &) = default;

  RocksCache(RocksCache &&) = default;

  RocksCache &operator=(RocksCache &&) = default;
};

}  // namespace kvfs
#endif //KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP
