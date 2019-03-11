/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_handler.cpp
 */

#include <rocksdb/slice_transform.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include "kvfs_rocksdb_handler.h"
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
//  options.OptimizeLevelStyleCompaction();

  // Enable prefix bloom for mem tables
//  options.prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(3));
//  options.memtable_prefix_bloom_bits = 100000000;
//  options.memtable_prefix_bloom_probes = 6;

  // Enable prefix bloom for SST files
//  rocksdb::BlockBasedTableOptions table_options = rocksdb::BlockBasedTableOptions();
//  table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, true));
//  options.table_factory.reset(NewBlockBasedTableFactory(table_options));

  // Create the DB if it's not already present.
  options.create_if_missing = true;
  options.enable_pipelined_write = true;

  rocksdb::DB *dbRaw;

  auto status =
      rocksdb::DB::Open(options, dbPath, &dbRaw);
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
