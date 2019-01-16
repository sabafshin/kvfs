/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_cache.h
 */

#ifndef KVFS_ROCKSDB_CACHE_H
#define KVFS_ROCKSDB_CACHE_H

#include <inodes/inode_cache.h>
#include <rocksdb/cache.h>
#include <rocksdb/status.h>
#include "rocks_db_handler.hpp"
#include <memory>

namespace kvfs {

class rocksdb_cache : public inode_cache {
 protected:

  explicit rocksdb_cache(std::shared_ptr<Store> db);
  ~rocksdb_cache();

  inode_cache_handle *insert(const slice &key,
                             const slice &value) override;

  inode_cache_handle *get(const slice &key,
                          const inode_access_mode &mode) override;

  void release(inode_cache_handle *handle) override;

  void write_back(inode_cache_handle *handle) override;

  void batch_commit(inode_cache_handle *handle1,
                    inode_cache_handle *handle2) override;

  void evict(const slice &key) override;

  static void clean_inode_handle(const rocksdb::Slice &key, void *value);

  static std::shared_ptr<Store> store;

 private:
  RocksCache rc;
};
}  // namespace kvfs
#endif //KVFS_ROCKSDB_CACHE_H
