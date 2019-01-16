/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_entry.hpp
 */

#ifndef KVFS_STORE_ENTRY_HPP
#define KVFS_STORE_ENTRY_HPP

#include "slice.hpp"
#include "store_result.hpp"

#include <sys/stat.h>
#include <sys/types.h>

namespace kvfs {
enum class StoreEntryType : uint8_t {
  DATA_KEY, F_KEY
};

typedef uint32_t kvfs_file_hash_t;
typedef uint64_t kvfs_file_block_number_t;

#ifdef __USE_LARGEFILE64
typedef struct stat64 kvfs_stat;
typedef ino64_t kvfs_file_inode_t;
#else
typedef ino_t kvfs_file_inode_t;
typedef struct stat kvfs_stat;
#endif

struct data_key {
  kvfs_file_inode_t inode;
  kvfs_file_hash_t hash;
  kvfs_file_block_number_t block_number;

  slice to_slice() const {
    return slice(reinterpret_cast<const char *>(this), sizeof(data_key));
  }
};

struct dir_key {
  kvfs_file_inode_t inode;
  kvfs_file_hash_t hash;

  slice to_slice() const {
    return slice(reinterpret_cast<const char *>(this), sizeof(dir_key));
  };
};

struct dir_value {
  char name[256];
  kvfs_file_inode_t this_inode;
  kvfs_file_hash_t parent_hash;
  uint64_t hardlink_count;
  struct data_key blocks_ptr;
  kvfs_stat fstat;
  char inline_data[4096];

  void parse(const StoreResult &s);
  slice to_slice() const {
    return slice(reinterpret_cast<const char *>(this), sizeof(dir_value));
  }
};

}  // namespace kvfs
#endif //KVFS_STORE_ENTRY_HPP
