/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.h
 */

#ifndef KVFS_DIRECTORY_ENTRY_H
#define KVFS_DIRECTORY_ENTRY_H

#include <list>
#include <unordered_map>
#include <store/store_entry.hpp>
#include <kvfs_utils/mutex.h>
#include <kvfs_utils/mutex_lock.h>

namespace kvfs {
struct dentry_cache_comp {
  bool operator()(const dir_key &lhs, const dir_key &rhs) const {
    return (lhs.hash == rhs.hash) && (lhs.inode == rhs.inode);
  }
};

struct dentry_cache_hash {
  std::size_t operator()(const dir_key &x) const {
    std::size_t seed = 0;
    seed ^= x.inode + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class dentry_cache {
 public:
  typedef std::pair<dir_key, dir_value> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<dir_key, std::list<Entry>::iterator,
                             dentry_cache_hash, dentry_cache_comp> CacheMap;

  explicit dentry_cache(size_t size)
      : maxsize(size) {}

  bool find(dir_key &key, dir_value &value);

  void insert(dir_key &key, dir_value &value);

  void evict(dir_key &key);

  void get_size(size_t &size_cache_list, size_t &size_cache_map) {
    size_cache_map = cache.size();
    size_cache_list = lookup.size();
  }

  ~dentry_cache() {
    delete cache_mutex;
  };

 private:
  size_t maxsize;
  CacheList cache;
  CacheMap lookup;

  Mutex *cache_mutex;
};
}  // namespace kvfs
#endif //KVFS_DIRECTORY_ENTRY_H
