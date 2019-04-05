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

#include <kvfs_store/kvfs_store_entry.h>
#include <mutex>
#include <list>
#include <unordered_map>
#include <memory>

namespace kvfs {

struct kvfsFileHandle {
  kvfsInodeKey key_;
  kvfsInodeValue md_;
  int flags_{};
  kvfs_off_t offset_{};

  kvfsFileHandle() = default;
  kvfsFileHandle(const kvfsInodeKey &key, const kvfsInodeValue &md, int flags)
      : key_(key), md_(md), flags_(flags) {};
};

struct OpenFilesCacheComparator {
  bool operator()(const int &lhs, const int &rhs) const;
};

struct OpenFilesCacheHash {
  std::size_t operator()(const int &x) const;
};

class OpenFilesCache {
 public:
  typedef std::pair<int, kvfsFileHandle> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<int, std::list<Entry>::iterator,
                             OpenFilesCacheHash, OpenFilesCacheComparator> CacheMap;

  explicit OpenFilesCache(size_t size);

  bool Find(const int &filedes, kvfsFileHandle &value);

  void Insert(const int &filedes, kvfsFileHandle &value);

  void Evict(const int &filedes);

  void Size(size_t &size_cache_list, size_t &size_cache_map);

  ~OpenFilesCache();;

 private:
  size_t maxsize_;
  CacheList cache_;
  CacheMap lookup_;
  std::unique_ptr<std::mutex> mutex_;

};
}  // namespace kvfs
#endif //KVFS_DIRECTORY_ENTRY_CACHE_H
