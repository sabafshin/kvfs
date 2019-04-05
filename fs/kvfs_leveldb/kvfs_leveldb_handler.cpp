/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_handler.cpp
 */

#include "kvfs_leveldb_handler.h"

kvfs::LevelDBHandles::LevelDBHandles(const std::string &dbPath) {
  leveldb::Options options;
  options.create_if_missing = true;
//  options.block_size = 4096; //4KiB
//  options.max_file_size = 67108864; // 64MiB
//  options.block_cache = leveldb::NewLRUCache(1000000);
  options.paranoid_checks = false;
  options.compression = leveldb::CompressionType::kSnappyCompression;
  options.reuse_logs = true;
//  options.write_buffer_size = 134217728; // 128MiB

  leveldb::DB *db_raw;
  auto status = leveldb::DB::Open(options, dbPath, &db_raw);
  if (!status.ok()) {
    throw LevelDBException(status, "Failed to open DB at the given path");
  }
  db.reset(db_raw);
}
kvfs::LevelDBHandles::~LevelDBHandles() {
  db.reset();
}