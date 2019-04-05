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

#include "kvfs_leveldb_exception.h"
#include <kvfs_config.h>
#include <memory>
#include <string>
#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>
#include <leveldb/cache.h>

namespace kvfs {

struct LevelDBHandles {
  std::unique_ptr<leveldb::DB> db;

  ~LevelDBHandles();

  explicit LevelDBHandles(const std::string &dbPath);

  LevelDBHandles(const LevelDBHandles &) = delete;
  LevelDBHandles &operator=(const LevelDBHandles &) = delete;
  LevelDBHandles(LevelDBHandles &&) = default;
  LevelDBHandles &operator=(LevelDBHandles &&) = default;
};
}

#endif //KVFS_KVFS_LEVELDB_HANDLER_H
