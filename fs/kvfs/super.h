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
#include "store/store_entry.h"

namespace kvfs {
struct SuperBlock {
  uint64_t next_free_inode_;
  uint64_t total_inode_count_;
  uint64_t total_block_count_;
  uint fs_number_of_mounts_;
  time_t fs_last_mount_time_;
  time_t fs_creation_time_;
  time_t fs_last_access_time_;
  time_t fs_last_modification_time_;
  kvfsDirKey store_end_key;

  void parse(const StoreResult &sr) {
    auto bytes_ = sr.asString();
    if (bytes_.size() != sizeof(SuperBlock)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes_.data();
    memmove(this, idx, sizeof(SuperBlock));
  }
  std::string to_string() const {
    std::string bytes_;
    return bytes_.append(reinterpret_cast<const char *>(this), sizeof(SuperBlock));
  }
};

struct FreeBlocksKey {
  char name[2];
  uint64_t number_;

  std::string to_string() const {
    std::string bytes_;
    return bytes_.append(reinterpret_cast<const char *>(this), sizeof(FreeBlocksKey));
  }
};
struct FreeBlocksValue {
  FreeBlocksKey next_key_;
  uint64_t blocks[512];

  std::string to_string() const {
    std::string bytes_;
    return bytes_.append(reinterpret_cast<const char *>(this), sizeof(FreeBlocksValue));
  }

  void parse(const StoreResult &sr) {
    auto bytes_ = sr.asString();
    if (bytes_.size() != sizeof(FreeBlocksValue)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes_.data();
    memmove(this, idx, sizeof(FreeBlocksValue));
  }
};

}  // namespace kvfs

#endif //KVFS_SUPERBLOCK_H