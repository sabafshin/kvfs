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

#include <store/store.hpp>
#include <store/store_entry.hpp>
#include <store/store_result.hpp>

#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/db.h>

namespace kvfs {

class RocksDBStore : public Store {
 public:
  explicit RocksDBStore(const string &_db_path);

  ~RocksDBStore();

 protected:
  void close() override;

  bool put(const std::string &key, const std::string &value) override;

  bool merge(const std::string &key, const std::string &value) override;

  StoreResult get(const std::string &key) override;

  bool delete_(const std::string &key) override;
  bool delete_range(const std::string &start, const std::string &end) override;

  std::vector<StoreResult> get_children(const std::string &key) override;
  StoreResult get_parent(const std::string &key) override;

  bool hasKey(const std::string &key) const override;

  bool sync() override;

  bool compact() override;

  std::unique_ptr<WriteBatch> beginWrite(size_t buf_size = 0) override;

 private:
  std::shared_ptr<RocksHandles> db_handle;
};
} //namespace kvfs
#endif //KVFS_ROCKSDB_STORE_HPP
