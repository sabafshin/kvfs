/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   InodeCache.cpp
 */

#include "inode_cache.h"

namespace kvfs {
void InodeCache::insert(const kvfsDirKey &key,
                        const kvfsMetaData &value) {
  MutexLock lock;

  auto handle = InodeCacheEntry(key, value, INODE_WRITE);
  Entry entry(key, handle);
  cache_list_.push_front(entry);
  cache_map_lookup_[key] = cache_list_.begin();
  if (cache_list_.size() > max_size_) {
    auto handle = cache_list_.back().second;
    clean_inode_handle(handle);
    cache_map_lookup_.erase(cache_list_.back().first);
    cache_list_.pop_back();
  }
}

bool InodeCache::get(const kvfsDirKey &key, InodeAccessMode mode, InodeCacheEntry &handle) {
  MutexLock lock;

  auto it = cache_map_lookup_.find(key);
  if (it == cache_map_lookup_.end()) {
    if (!store_->hasKey(key.pack())) {
      return false;
    }
    StoreResult sr = store_->get(key.pack());
    if (sr.isValid()) {
      kvfsMetaData md_;
      md_.parse(sr);
      handle = InodeCacheEntry(key, md_, mode);
      this->insert(key, md_);
      return true;
    }
    // Something horribly went wrong here.
    return false;
  }
  handle = it->second->second;
  cache_list_.push_front(*(it->second));
  cache_list_.erase(it->second);
  cache_map_lookup_[key] = cache_list_.begin();

  return true;
}

void InodeCache::clean_inode_handle(InodeCacheEntry handle) {
  MutexLock lock;

  if (handle.access_mode == INODE_WRITE) {
    store_->put(handle.key_.pack(), handle.md_.pack());
  }
  if (handle.access_mode == INODE_DELETE) {
    store_->delete_(handle.key_.pack());
  }
}

void InodeCache::evict(const kvfsDirKey &key) {
  MutexLock lock;

  auto it = cache_map_lookup_.find(key);
  if (it != cache_map_lookup_.end()) {
    auto handle = it->second->second;
    clean_inode_handle(handle);
    cache_map_lookup_.erase(it);
  }
}

void InodeCache::write_back(InodeCacheEntry &handle) {
  MutexLock lock;
  if (handle.access_mode == INODE_WRITE) {
    store_->put(handle.key_.pack(), handle.md_.pack());
    handle.access_mode = INODE_READ;
  } else if (handle.access_mode == INODE_DELETE) {
    store_->delete_(handle.key_.pack());
    this->evict(handle.key_);
  } else if (handle.access_mode == INODE_RW) {
    store_->merge(handle.key_.pack(), handle.md_.pack());
  }
}
size_t InodeCache::size() {
  MutexLock lock;
  return cache_list_.size();
}
}  // namespace kvfs