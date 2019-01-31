#include <utility>

/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   InodeCache.h
 */

#ifndef KVFS_INODE_CACHE_H
#define KVFS_INODE_CACHE_H

#include <store/store.h>
#include <store/store_entry.h>
#include <kvfs_utils/mutex.h>
#include <kvfs_utils/mutex_lock.h>
#include <memory>
#include <string>
#include <list>
#include <unordered_map>

namespace kvfs {

enum InodeAccessMode : uint8_t {
  INODE_READ = 0,
  INODE_DELETE = 1,
  INODE_WRITE = 2,
};

enum InodeState : uint8_t {
  CLEAN = 0,
  DELETED = 1,
  DIRTY = 2,
};

struct InodeCacheEntry {
  StoreEntryKey key_;
  std::string value_;
  InodeState state;
  InodeAccessMode access_mode;

  InodeCacheEntry(const StoreEntryKey &key, std::string value, InodeAccessMode mode)
      : key_(key), value_(std::move(value)), access_mode(mode) {}
  InodeCacheEntry() : access_mode(INODE_READ) {}
};

struct InodeCacheComparator {
  bool operator()(const StoreEntryKey &lhs, const StoreEntryKey &rhs) const {
    return (lhs.hash_ == rhs.hash_) && (lhs.inode_ == rhs.inode_) && (lhs.block_number_ == rhs.block_number_);
  }
};

struct InodeCacheHash {
  std::size_t operator()(const StoreEntryKey &x) const {
    std::size_t seed = 0;
    seed ^= x.inode_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.hash_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= x.block_number_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

class InodeCache {
 public:
  typedef std::pair<StoreEntryKey, InodeCacheEntry> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<StoreEntryKey, std::list<Entry>::iterator,
                             InodeCacheHash, InodeCacheComparator> CacheMap;

  explicit InodeCache(size_t size, const std::shared_ptr<Store> store)
      : max_size_(size), store_(store) {}

  void insert(const StoreEntryKey &key, const std::string &value);

  bool get(const StoreEntryKey &key, InodeAccessMode mode, InodeCacheEntry &handle);

  void write_back(InodeCacheEntry &key);

  void evict(const StoreEntryKey &key);

  size_t size();

  ~InodeCache() = default;

 private:
  CacheList cache_list_;
  CacheMap cache_map_lookup_;
  size_t max_size_;

  std::shared_ptr<Store> store_;

  void clean_inode_handle(InodeCacheEntry handle);
};

}  // namespace kvfs

#endif //KVFS_INODE_CACHE_H