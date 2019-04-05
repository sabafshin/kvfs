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

#include <kvfs_store/kvfs_store_result.h>
#include <kvfs_config.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstring>
#include <time.h>
#include <ctime>
#include <fcntl.h>
#include <zconf.h>
#include <kvfs/kvfs_dirent.h>
#include <filesystem>

namespace kvfs {

struct kvfsInodeKey {
  kvfs_file_inode_t inode_{};
  kvfs_file_hash_t hash_{};

  void parse(const std::string &sr);
  std::string pack() const;

  bool operator==(const kvfsInodeKey &c2) const;
  bool operator!=(const kvfsInodeKey &c2) const;
};

struct kvfsBlockKey {
  kvfs_file_inode_t inode_{};
  kvfs_off_t block_number_{};
  byte __padding[2]{}; // to distinguish this key from kvfsInodeKey

  kvfsBlockKey(kvfs_file_inode_t inode, kvfs_off_t number);
  kvfsBlockKey() = default;

  std::string pack() const;

  void parse(const std::string &sr);

  bool operator==(const kvfsBlockKey &c2) const;
};

struct kvfsBlockValue {
  kvfsBlockKey next_block_{};
  size_t size_{};
  byte data[KVFS_DEF_BLOCK_SIZE]{};

  // write append upto buffer_size, buffer_size is <= KVFS_BLOCK_SIZE
  const void *write(const void *buffer, size_t buffer_size);
  // write at give offset upto buffer size
  const void *write_at(const void *buffer, size_t buffer_size, kvfs_off_t offset);

  // read into buffer upto size
  void *read(void *buffer, size_t size) const;
  // read from given offset upto size
  void *read_at(void *buffer, size_t size, kvfs_off_t offset) const;

  std::string pack() const;
  void parse(const KVStoreResult &sr);
};

struct kvfsInodeValue {
  kvfs_dirent dirent_{};
  kvfs_stat fstat_{};
  kvfsInodeKey real_key_{}; // to mark hardlinks to real inode

  kvfsInodeValue() = default;

  kvfsInodeValue(const std::string &name,
                 const kvfs_file_inode_t &inode,
                 const mode_t &mode,
                 const kvfsInodeKey &real_key);

  void parse(const kvfs::KVStoreResult &result);
  std::string pack() const;
};

}  // namespace kvfs
#endif //KVFS_STORE_ENTRY_H