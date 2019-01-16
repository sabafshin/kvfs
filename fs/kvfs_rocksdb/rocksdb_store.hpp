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

#include "rocks_db_handler.hpp"
#include "rocks_db_exception.hpp"
#include "rocksdb_slice.hpp"

#include <store/store.hpp>
#include <store/store_entry.hpp>
#include <store/slice.hpp>

#include <rocksdb/db.h>

namespace kvfs {

class RocksDBStore : public Store {
 public:
  explicit RocksDBStore(const string &_db_path);

  ~RocksDBStore();

 protected:
  void close() override;

  bool put(const slice &key, const slice &value) override;

  bool merge(const slice &key, const slice &value) override;

  StoreResult get(const slice &key) override;

  bool delete_(const slice &key) override;
  bool delete_range(const slice &start, const slice &end) override;

  std::vector<StoreResult> get_children(const slice &key) override;
  bool get_parent(const slice &key) override;

  bool hasKey(const slice &key) const override;

  bool sync() override;

  bool compact() override;

  std::unique_ptr<WriteBatch> beginWrite(size_t buf_size = 0) override;

 private:
  RocksHandles db_handle;
};
} //namespace kvfs
#endif //KVFS_ROCKSDB_STORE_HPP
