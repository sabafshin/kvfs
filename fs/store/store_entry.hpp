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

#include <store/store_result.hpp>
#include <kvfs_config.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

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

  std::string to_string() const {
    std::string value;
    return value.append(reinterpret_cast<const char *>(this), sizeof(data_key));
  }
};

struct dir_key {
  kvfs_file_inode_t inode;
  kvfs_file_hash_t hash;

  std::string to_string() const {
    std::string value;
    return value.append(reinterpret_cast<const char *>(this), sizeof(dir_key));
  };
};

struct dir_value {
  char name[KVFS_NAME_LEN + 1];
  kvfs_file_inode_t this_inode;
  kvfs_file_hash_t parent_hash;
  uint64_t hardlink_count;
  struct data_key blocks_ptr;
  kvfs_stat fstat;
  char inline_data[KVFS_DEF_BLOCK_SIZE_4K];

  void parse(const kvfs::StoreResult &result) {
    auto bytes = result.asString();
    if (bytes.size() != sizeof(dir_value)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes.data();
    memmove(this, idx, sizeof(dir_value));
  }

  std::string to_string() const {
    std::string value;
    return value.append(reinterpret_cast<const char *>(this), sizeof(dir_value));
  }
};

}  // namespace kvfs
#endif //KVFS_STORE_ENTRY_HPP
