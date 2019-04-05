/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   superblock.h
 */

#ifndef KVFS_SUPERBLOCK_H
#define KVFS_SUPERBLOCK_H

#include <string>
#include <time.h>
#include <kvfs/fs_error.h>
#include <kvfs_store/kvfs_store_entry.h>

namespace kvfs {

struct kvfsSuperBlock {
  uint64_t next_free_inode_{};
  uint64_t total_inode_count_{};
  uint fs_number_of_mounts_{};
  time_t fs_last_mount_time_{};
  time_t fs_creation_time_{};
  time_t fs_last_access_time_{};
  time_t fs_last_modification_time_{};
  size_t freed_inodes_count_{};

  void parse(const KVStoreResult &sr);
  std::string pack() const;
};

struct kvfsFreedInodesKey {
  char name[11];
  uint64_t number_;

  std::string pack() const;
};
struct kvfsFreedInodesValue {
  uint32_t count_{};
  kvfs_file_inode_t inodes[512]{};

  std::string pack() const;
  void parse(const KVStoreResult &sr);
};

}  // namespace kvfs

#endif //KVFS_SUPERBLOCK_H