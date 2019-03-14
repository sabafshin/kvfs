/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_handler.cpp
 */

#include "kvfs_leveldb_handler.h"

kvfs::LevelDBHandles::LevelDBHandles(std::string dbPath) {
  leveldb::Options options;
  options.create_if_missing = true;

  leveldb::DB *db_raw;
  auto status = leveldb::DB::Open(options, dbPath, &db_raw);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open DB at the given path");
  }
  db.reset(db_raw);
}
kvfs::LevelDBHandles::~LevelDBHandles() {
  db.reset();
}

