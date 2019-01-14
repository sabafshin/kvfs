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

#include <string>

using std::string;

#include <sys/stat.h>
#include <sys/types.h>
#include "slice.hpp"

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
    return slice((const char *) this, sizeof(data_key));
  }
};

struct dir_key {
  kvfs_file_inode_t inode;
  kvfs_file_hash_t hash;

  slice to_slice() const {
    return slice((const char *) this, sizeof(dir_key));
  }
};

struct dir_value {
  string name;
  kvfs_file_hash_t parent_name;
  kvfs_file_inode_t this_inode;
  kvfs_stat fstat;
  uint64_t hardlink_count;
  struct data_key blocks_ptr;
  char inline_data[4096];

  slice to_slice() const {
    return slice((const char *) this, sizeof(dir_value));
  }
};

}
#endif //KVFS_STORE_ENTRY_HPP
