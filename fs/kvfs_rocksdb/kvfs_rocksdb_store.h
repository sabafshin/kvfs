/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.hpp
 */

#ifndef KVFS_ROCKSDB_STORE_HPP
#define KVFS_ROCKSDB_STORE_HPP

#include "kvfs_rocksdb_handler.h"
#include "kvfs_rocksdb_exception.h"

#include <kvfs_store/kvfs_store.h>
#include <kvfs_store/kvfs_store_entry.h>
#include <kvfs_store/kvfs_store_result.h>
#include <rocksdb/options.h>
#include <rocksdb/db.h>

namespace kvfs {

class kvfsRocksDBStore : public KVStore {
 public:
  explicit kvfsRocksDBStore(const std::string &_db_path);

  ~kvfsRocksDBStore();

 protected:
  void Close() override;

  bool Put(const std::string &key, const std::string &value) override;

  bool Merge(const std::string &key, const std::string &value) override;

  KVStoreResult Get(const std::string &key) override;

  bool Delete(const std::string &key) override;
  bool DeleteRange(const std::string &start, const std::string &end) override;

  std::vector<KVStoreResult> GetChildren(const std::string &key) override;
  KVStoreResult GetParent(const std::string &key) override;

  bool Sync() override;

  bool Compact() override;

  bool Destroy() override;

  std::unique_ptr<WriteBatch> GetWriteBatch() override;

  std::unique_ptr<Iterator> GetIterator() override;

 private:
  std::shared_ptr<RocksHandles> db_handle;
};
} //namespace kvfs
#endif //KVFS_ROCKSDB_STORE_HPP
