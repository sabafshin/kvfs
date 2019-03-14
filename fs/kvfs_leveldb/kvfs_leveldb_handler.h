/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_handler.h
 */

#ifndef KVFS_KVFS_LEVELDB_HANDLER_H
#define KVFS_KVFS_LEVELDB_HANDLER_H

#include <memory>
#include <string>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>

namespace kvfs {

struct LevelDBHandles {
  std::unique_ptr<leveldb::DB> db;

  ~LevelDBHandles();

  explicit LevelDBHandles(std::string dbPath);

  LevelDBHandles(const LevelDBHandles &) = delete;
  LevelDBHandles &operator=(const LevelDBHandles &) = delete;
  LevelDBHandles(LevelDBHandles &&) = default;
  LevelDBHandles &operator=(LevelDBHandles &&) = default;
};
}

#endif //KVFS_KVFS_LEVELDB_HANDLER_H
