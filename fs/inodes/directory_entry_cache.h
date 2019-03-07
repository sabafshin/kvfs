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
#include <mutex>
#include <list>
#include <unordered_map>
#include <memory>

namespace kvfs {

struct kvfsFileHandle {
  kvfsDirKey key_{};
  kvfsMetaData md_;
  int flags_{};
  kvfs_off_t offset_{};

  kvfsFileHandle() = default;
  kvfsFileHandle(const kvfsDirKey &key, const kvfsMetaData &md, int flags)
      : key_(key), md_(md), flags_(flags) {};
};

struct DentryCacheComparator {
  bool operator()(const int &lhs, const int &rhs) const {
    return (lhs == rhs);
  }
};

struct DentryCacheHash {
  std::size_t operator()(const int &x) const {
    return static_cast<size_t>(x);
  }
};

class DentryCache {
 public:
  typedef std::pair<int, kvfsFileHandle> Entry;
  typedef std::list<Entry> CacheList;
  typedef std::unordered_map<int, std::list<Entry>::iterator,
                             DentryCacheHash, DentryCacheComparator> CacheMap;

  explicit DentryCache(size_t size)
      : maxsize_(size), mutex_(std::make_unique<std::mutex>()) {}

  bool find(const int &filedes, kvfsFileHandle &value);

  void insert(const int &filedes, kvfsFileHandle &value);

  void evict(const int &filedes);

  void size(size_t &size_cache_list, size_t &size_cache_map) {
    size_cache_map = cache_.size();
    size_cache_list = lookup_.size();
  }

  ~DentryCache() {
    mutex_.reset();
  };

 private:
  size_t maxsize_;
  CacheList cache_;
  CacheMap lookup_;
  std::unique_ptr<std::mutex> mutex_;

};
}  // namespace kvfs
#endif //KVFS_DIRECTORY_ENTRY_CACHE_H
