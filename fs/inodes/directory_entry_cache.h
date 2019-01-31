/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.h
 */

#ifndef KVFS_DIRECTORY_ENTRY_CACHE_H
#define KVFS_DIRECTORY_ENTRY_CACHE_H

#include <store/store_entry.h>
#include <kvfs_utils/mutex.h>
#include <kvfs_utils/mutex_lock.h>

#include <list>
#include <unordered_map>

namespace kvfs {
struct DentryCacheComparator {
  bool operator()(const StoreEntryKey &lhs, const StoreEntryKey &rhs) const {
    return (lhs.hash_ == rhs.hash_) && (lhs.inode_ == rhs.inode_);
  }
};

struct DentryCacheHash {
  std::size_t operator()(const StoreEntryKey &x) const {
    std::size_t seed = 0;
    seed ^= x.inode_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.hash_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class DentryCache {
 public:
  typedef std::pair<StoreEntryKey, StoreEntryValue> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<StoreEntryKey, std::list<Entry>::iterator,
                             DentryCacheHash, DentryCacheComparator> CacheMap;

  explicit DentryCache(size_t size)
      : maxsize_(size) {}

  bool find(StoreEntryKey &key, StoreEntryValue &value);

  void insert(StoreEntryKey &key, StoreEntryValue &value);

  void evict(StoreEntryKey &key);

  void size(size_t &size_cache_list, size_t &size_cache_map) {
    size_cache_map = cache_.size();
    size_cache_list = lookup_.size();
  }

  ~DentryCache() = default;

 private:
  size_t maxsize_;
  CacheList cache_;
  CacheMap lookup_;

};
}  // namespace kvfs
#endif //KVFS_DIRECTORY_ENTRY_CACHE_H
