#include <utility>

/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_cache.h
 */

#ifndef KVFS_INODE_CACHE_H
#define KVFS_INODE_CACHE_H

#include <store/store.hpp>
#include <store/store_entry.hpp>
#include <kvfs_utils/mutex.h>
#include <kvfs_utils/mutex_lock.h>
#include <memory>
#include <string>
#include <list>
#include <unordered_map>

namespace kvfs {

enum inode_access_mode : uint8_t {
  INODE_READ = 0,
  INODE_DELETE = 1,
  INODE_WRITE = 2,
};

enum inode_state : uint8_t {
  CLEAN = 0,
  DELETED = 1,
  DIRTY = 2,
};

struct inode_cache_handle {
  kvfs_file_inode_t inode;
  kvfs_file_hash_t hash;
  kvfs_file_block_number_t blockN;
  bool isDir;
  std::string value;
  inode_state state;
  inode_access_mode access_mode;

  inode_cache_handle(const dir_key &key, std::string value, inode_access_mode mode)
      : inode(key.inode), hash(key.hash), value(std::move(value)), access_mode(mode), isDir(true) {}
  inode_cache_handle(const data_key &key, std::string value, inode_access_mode mode)
      : inode(key.inode), hash(key.hash), value(std::move(value)), access_mode(mode), isDir(false) {}
  inode_cache_handle() : access_mode(INODE_READ) {}
};

struct cache_comp {
  bool operator()(const data_key &lhs, const data_key &rhs) const {
    return (lhs.hash == rhs.hash) && (lhs.inode == rhs.inode) && (lhs.block_number == rhs.block_number);
  }
};

struct cache_hash {
  std::size_t operator()(const data_key &x) const {
    std::size_t seed = 0;
    seed ^= x.inode + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.block_number + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class inode_cache {
 public:
  typedef std::pair<data_key, inode_cache_handle> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<data_key, std::list<Entry>::iterator,
                             cache_hash, cache_comp> CacheMap;

  explicit inode_cache(size_t size, std::shared_ptr<Store> store)
      : max_size(size), store_(store) {}

  void insert(const data_key &key, const std::string &value);
  void insert(const dir_key &key, const std::string &value);

  bool get(const data_key &key, inode_access_mode mode, inode_cache_handle &handle);
  bool get(const dir_key &key, inode_access_mode mode, inode_cache_handle &handle);

  void write_back(inode_cache_handle &key);

  void evict(const data_key &key);
  void evict(const dir_key &key);

  size_t size();

  ~inode_cache() {
    store_.reset();
    delete i_cache_mutex;
  };

 private:
  CacheList cache;
  CacheMap lookup;
  size_t max_size;

  std::shared_ptr<Store> store_;

  kvfs::Mutex *i_cache_mutex;
  void clean_inode_handle(inode_cache_handle handle);
};

}  // namespace kvfs

#endif //KVFS_INODE_CACHE_H
