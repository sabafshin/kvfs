/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.cpp
 */

#include <kvfs/kvfs.h>
kvfs::KVFS::KVFS(std::string mount_path)
    : store_(std::make_shared<RocksDBStore>(mount_path)) {}
kvfs::KVFS::KVFS() : store_(std::make_shared<RocksDBStore>(root_path_)) {}
kvfs::KVFS::~KVFS() {
  store_.reset();
  inode_cache_.reset();
  dentry_cache_.reset();
}

