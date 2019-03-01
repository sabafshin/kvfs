/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_entry.hpp
 */

#ifndef KVFS_STORE_ENTRY_H
#define KVFS_STORE_ENTRY_H

#include <store/store_result.h>
#include <kvfs_config.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <time.h>
#include <ctime>
#include <fcntl.h>
#include <zconf.h>

namespace kvfs {
enum class StoreEntryType : uint8_t {
  DATA_KEY, F_KEY
};

typedef size_t kvfs_file_hash_t;
typedef unsigned char byte;
#ifdef __USE_LARGEFILE64
typedef struct dirent64 kvfs_dirent;
typedef struct stat64 kvfs_stat;
typedef ino64_t kvfs_file_inode_t;
#else
typedef struct dirent kvfs_dirent;
typedef ino_t kvfs_file_inode_t;
typedef struct stat kvfs_stat;
#endif

struct kvfsDirKey {
  kvfs_file_inode_t inode_;
  kvfs_file_hash_t hash_;

  std::string pack() const {
    std::string d(sizeof(kvfsDirKey), L'\0');
    memcpy(&d[0], this, d.size());
    return d;
  }
};

struct kvfsBlockKey {
  uint64_t block_number_;

  std::string pack() const {
    std::string d(sizeof(kvfsBlockKey), L'\0');
    memcpy(&d[0], this, d.size());
    return d;
  }

  bool operator==(const kvfsBlockKey &c2) const {
    return c2.block_number_ == this->block_number_;
  }
};

struct kvfsBlockValue {
  kvfsBlockKey next_block_;
  size_t size_;
  byte data[KVFS_DEF_BLOCK_SIZE_4K];

  std::string pack() const {
    std::string d(sizeof(kvfsBlockValue), L'\0');
    memcpy(&d[0], this, d.size());
    return d;
  }

  void parse(const StoreResult &sr) {
    auto bytes = sr.asString();
    if (bytes.size() != sizeof(kvfsBlockValue)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes.data();
    memmove(this, idx, sizeof(kvfsBlockValue));
  }
};

struct kvfsMetaData {
  kvfs_dirent dirent_{};
  kvfsDirKey parent_key_{};
  kvfs_stat fstat_{};
  kvfsBlockKey first_block_key_{};
  kvfsBlockKey last_block_key_{};
  kvfsDirKey real_key_{};
  kvfsBlockValue inline_blck{};


  kvfsMetaData() = default;

  kvfsMetaData(const std::string &name,
               const kvfs_file_inode_t &inode,
               const mode_t &mode,
               const kvfsDirKey &current_key_) {
    name.copy(dirent_.d_name, name.length());
    // get a free inode for this new file
    dirent_.d_ino = inode;
    fstat_.st_ino = dirent_.d_ino;
    // generate stat
    fstat_.st_mode = mode;
    fstat_.st_blocks = 0;
    fstat_.st_ctim.tv_sec = std::time(nullptr);
    fstat_.st_blksize = KVFS_DEF_BLOCK_SIZE_4K;
    // owner
    fstat_.st_uid = getuid();
    fstat_.st_gid = getgid();
    // link count
    if (S_ISREG(mode)) {
      fstat_.st_nlink = 1;
    } else {
      fstat_.st_nlink = 2;
    }
    fstat_.st_blocks = 0;
    fstat_.st_size = 0;
    // set parent
    parent_key_ = current_key_;
  }
  
  void parse(const kvfs::StoreResult &result) {
    auto bytes = result.asString();
    if (bytes.size() != sizeof(kvfsMetaData)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes.data();
    memmove(this, idx, sizeof(kvfsMetaData));
  }

  std::string pack() const {
    std::string d(sizeof(kvfsMetaData), L'\0');
    memcpy(&d[0], this, d.size());
    return d;
  }
};

}  // namespace kvfs
#endif //KVFS_STORE_ENTRY_H