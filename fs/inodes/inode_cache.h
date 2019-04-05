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

#include <kvfs_store/kvfs_store.h>
#include <kvfs_store/kvfs_store_entry.h>
#include <memory>
#include <string>
#include <list>
#include <mutex>
#include <unordered_map>

namespace kvfs {

enum InodeAccessMode : uint8_t {
  INODE_READ = 0,
  INODE_DELETE = 1,
  INODE_WRITE = 2,
  INODE_RW = 3
};

enum InodeState : uint8_t {
  CLEAN = 0,
  DELETED = 1,
  DIRTY = 2,
};

struct InodeCacheEntry {
  kvfsInodeKey key_;
  kvfsInodeValue md_;
  InodeState state;
  InodeAccessMode access_mode;

  InodeCacheEntry(const kvfsInodeKey &key, kvfsInodeValue value, InodeAccessMode mode)
      : key_(key), md_(value), access_mode(mode) {}
  InodeCacheEntry() : access_mode(INODE_READ) {}
};

struct InodeCacheComparator {
  bool operator()(const kvfsInodeKey &lhs, const kvfsInodeKey &rhs) const {
    return (lhs.hash_ == rhs.hash_) && (lhs.inode_ == rhs.inode_);
  }
};

struct InodeCacheHash {
  std::size_t operator()(const kvfsInodeKey &x) const;
};

class InodeCache {
 public:
  typedef std::pair<kvfsInodeKey, InodeCacheEntry> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<kvfsInodeKey, std::list<Entry>::iterator,
                             InodeCacheHash, InodeCacheComparator> CacheMap;

  explicit InodeCache(size_t size, std::shared_ptr<KVStore> store);

  void Insert(const kvfsInodeKey &key, const kvfsInodeValue &value);

  bool Get(const kvfsInodeKey &key, InodeAccessMode mode, InodeCacheEntry &handle);

  void WriteBack(InodeCacheEntry &key);

  void Evict(const kvfsInodeKey &key);

  size_t Size();

  ~InodeCache() = default;

 private:
  CacheList cache_list_;
  CacheMap cache_map_lookup_;
  size_t max_size_;

  std::shared_ptr<KVStore> store_;
  std::unique_ptr<std::mutex> mutex_;

  void CleanHandle(InodeCacheEntry handle);
};

}  // namespace kvfs

#endif //KVFS_INODE_CACHE_H