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

#include <store/store.h>
#include <store/store_entry.h>
#include <store/store_result.h>
#include "kvfs_leveldb_exception.h"
#include "kvfs_leveldb_handler.h"

namespace kvfs {

class kvfsLevelDBStore : public Store {
 public:
  explicit kvfsLevelDBStore(const std::string &db_path);
  ~kvfsLevelDBStore();

 protected:
  void close() override;

  bool put(const std::string &key, const std::string &value) override;

  bool merge(const std::string &key, const std::string &value) override;

  StoreResult get(const std::string &key) override;

  bool delete_(const std::string &key) override;
  bool delete_range(const std::string &start, const std::string &end) override;

  std::vector<StoreResult> get_children(const std::string &key) override;
  StoreResult get_parent(const std::string &key) override;

  bool sync() override;

  bool compact() override;

  bool destroy() override;

  std::unique_ptr<WriteBatch> get_write_batch() override;

  std::unique_ptr<Iterator> get_iterator() override;

 private:
  std::shared_ptr<LevelDBHandles> db_handle;
  std::string db_name;
};
}
#endif //KVFS_KVFS_LEVELDB_STORE_H