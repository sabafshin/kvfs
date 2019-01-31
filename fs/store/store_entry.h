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

#include <store/store_result.h>
#include <kvfs_config.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>

namespace kvfs {
enum class StoreEntryType : uint8_t {
  DATA_KEY, F_KEY
};

typedef uint32_t kvfs_file_hash_t;
typedef uint64_t kvfs_file_block_number_t;

#ifdef __USE_LARGEFILE64
typedef struct dirent64 kvfs_dirent;
typedef struct stat64 kvfs_stat;
typedef ino64_t kvfs_file_inode_t;
#else
typedef struct dirent kvfs_dirent;
typedef ino_t kvfs_file_inode_t;
typedef struct stat kvfs_stat;
#endif

struct StoreEntryKey {
  kvfs_file_inode_t inode_;
  kvfs_file_hash_t hash_;
  kvfs_file_block_number_t block_number_;

  std::string to_string() const {
    std::string bytes_;
    return bytes_.append(reinterpret_cast<const char *>(this), sizeof(StoreEntryKey));
  }
};

struct StoreEntryValue {
  kvfs_dirent dirent_;
  kvfs_file_hash_t parent_hash_;
  kvfs_stat fstat_;
  char inline_data[KVFS_DEF_BLOCK_SIZE_4K];

  void parse(const kvfs::StoreResult &result) {
    auto bytes = result.asString();
    if (bytes.size() != sizeof(StoreEntryValue)) {
      throw std::invalid_argument("Bad size");
    }
    auto *idx = bytes.data();
    memmove(this, idx, sizeof(StoreEntryValue));
  }

  std::string to_string() const {
    std::string bytes_;
    return bytes_.append(reinterpret_cast<const char *>(this), sizeof(StoreEntryValue));
  }
};

}  // namespace kvfs
#endif //KVFS_STORE_ENTRY_HPP