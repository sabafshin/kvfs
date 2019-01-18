/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_handler.cpp
 */

#include "rocks_db_handler.hpp"
namespace kvfs {

RocksHandles::~RocksHandles() {
//  auto status = db->Close();
//
//  if (!status.ok()) {
//    db->SyncWAL();
//  }
  db.reset();
}

RocksHandles::RocksHandles(string dbPath) {

  Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
  options.IncreaseParallelism();

  // Create the DB if it's not already present.
  options.create_if_missing = true;

  rocksdb::TransactionDB *dbRaw;

  auto status =
      rocksdb::TransactionDB::Open(options, rocksdb::TransactionDBOptions(), dbPath, &dbRaw);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open DB at the given path");
  }

  db.reset(dbRaw);
}

RocksIterator::~RocksIterator() {
  it.reset();
}

RocksIterator::RocksIterator(RocksHandles db) {
  it.reset(db.db->NewIterator(ReadOptions()));
}
RocksCache::~RocksCache() {
  cache.reset();
}
RocksCache::RocksCache() {
  cache = rocksdb::NewLRUCache(CACHE_SIZE, -1, true);
}
size_t RocksCache::get_size() {
  return cache->GetUsage() / cache->GetCapacity() % CACHE_SIZE;
}
}  // namespace kvfs
