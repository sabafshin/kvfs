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

RocksHandles::RocksHandles(const std::string &dbPath) {

  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
  options.IncreaseParallelism();
  options.enable_pipelined_write = true;
  options.paranoid_checks = false;
  options.compression = rocksdb::CompressionType::kNoCompression;
  options.enable_write_thread_adaptive_yield = true;
  options.allow_concurrent_memtable_write = true;
  options.allow_ingest_behind = true;
  options.allow_mmap_reads = true;
  options.allow_mmap_writes = true;
  options.allow_2pc = true;
  options.allow_fallocate = true;
  options.optimize_filters_for_hits = true;
  options.paranoid_file_checks = false;
  options.arena_block_size = 4096;
  options.use_adaptive_mutex = true;
  options.enable_thread_tracking = true;

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
  it.reset(db.db->NewIterator(rocksdb::ReadOptions()));
}
RocksCache::~RocksCache() {
  cache.reset();
}
RocksCache::RocksCache() {
  cache = rocksdb::NewLRUCache(KVFS_CACHE_SIZE, -1, true);
}
size_t RocksCache::get_size() {
  return cache->GetUsage() / cache->GetCapacity() % KVFS_CACHE_SIZE;
}
}  // namespace kvfs
