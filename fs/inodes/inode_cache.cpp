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
void InodeCache::insert(const StoreEntryKey &key,
                        const std::string &value) {
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

bool InodeCache::get(const StoreEntryKey &key, InodeAccessMode mode, InodeCacheEntry &handle) {
  MutexLock lock;

  auto it = cache_map_lookup_.find(key);
  if (it == cache_map_lookup_.end()) {
    StoreResult sr = store_->get(key.to_string());
    if (sr.isValid()) {
      handle = InodeCacheEntry(key, sr.asString(), mode);
      this->insert(key, sr.asString());
      return true;
    }
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
    store_->put(handle.key_.to_string(), handle.value_);
  }
  if (handle.access_mode == INODE_DELETE) {
    store_->put(handle.key_.to_string(), handle.value_);
  }
}

void InodeCache::evict(const StoreEntryKey &key) {
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
    store_->put(handle.key_.to_string(), handle.value_);
    handle.access_mode = INODE_READ;
  } else if (handle.access_mode == INODE_DELETE) {
    store_->put(handle.key_.to_string(), handle.value_);
    this->evict(handle.key_);
  }
}
size_t InodeCache::size() {
  MutexLock lock;
  return cache_list_.size();
}
}  // namespace kvfs