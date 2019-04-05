/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_store.h
 */

#ifndef KVFS_KVFS_LEVELDB_STORE_H
#define KVFS_KVFS_LEVELDB_STORE_H

#include <kvfs_store/kvfs_store.h>
#include <kvfs_store/kvfs_store_entry.h>
#include <kvfs_store/kvfs_store_result.h>
#include "kvfs_leveldb_exception.h"
#include "kvfs_leveldb_handler.h"

namespace kvfs {

class kvfsLevelDBStore : public KVStore {
 public:
  explicit kvfsLevelDBStore(const std::string &db_path);
  ~kvfsLevelDBStore();

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
  std::shared_ptr<LevelDBHandles> db_handle;
  std::string db_name;
};
}
#endif //KVFS_KVFS_LEVELDB_STORE_H