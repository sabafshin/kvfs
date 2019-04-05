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
void InodeCache::Insert(const kvfsInodeKey &key,
                        const kvfsInodeValue &value) {
  mutex_->lock();
  auto handle = InodeCacheEntry(key, value, INODE_WRITE);
  Entry entry(key, handle);
  cache_list_.push_front(entry);
  cache_map_lookup_[key] = cache_list_.begin();
  if (cache_list_.size() > max_size_) {
    auto handle = cache_list_.back().second;
    CleanHandle(handle);
    cache_map_lookup_.erase(cache_list_.back().first);
    cache_list_.pop_back();
  }
  mutex_->unlock();
}

bool InodeCache::Get(const kvfsInodeKey &key, InodeAccessMode mode, InodeCacheEntry &handle) {
  mutex_->lock();
  auto it = cache_map_lookup_.find(key);
  if (it == cache_map_lookup_.end()) {
    KVStoreResult sr = store_->Get(key.pack());
    if (sr.isValid()) {
      kvfsInodeValue md_;
      md_.parse(sr);
      handle = InodeCacheEntry(key, md_, mode);
      this->Insert(key, md_);
      mutex_->unlock();
      return true;
    }
    // Something horribly went wrong here.
    mutex_->unlock();
    return false;
  }
  handle = it->second->second;
  cache_list_.push_front(*(it->second));
  cache_list_.erase(it->second);
  cache_map_lookup_[key] = cache_list_.begin();

  mutex_->unlock();
  return true;
}

void InodeCache::CleanHandle(InodeCacheEntry handle) {
  mutex_->lock();
  if (handle.access_mode == INODE_WRITE) {
    store_->Put(handle.key_.pack(), handle.md_.pack());
  }
  if (handle.access_mode == INODE_DELETE) {
    store_->Delete(handle.key_.pack());
  }
  mutex_->unlock();
}

void InodeCache::Evict(const kvfsInodeKey &key) {
  mutex_->lock();
  auto it = cache_map_lookup_.find(key);
  if (it != cache_map_lookup_.end()) {
    auto handle = it->second->second;
    CleanHandle(handle);
    cache_map_lookup_.erase(it);
  }
  mutex_->unlock();
}

void InodeCache::WriteBack(InodeCacheEntry &key) {
  mutex_->lock();
  if (key.access_mode == INODE_WRITE) {
    store_->Put(key.key_.pack(), key.md_.pack());
    key.access_mode = INODE_READ;
  } else if (key.access_mode == INODE_DELETE) {
    store_->Delete(key.key_.pack());
    this->Evict(key.key_);
  } else if (key.access_mode == INODE_RW) {
    store_->Merge(key.key_.pack(), key.md_.pack());
  }
  mutex_->unlock();
}
size_t InodeCache::Size() {
  return cache_list_.size();
}
InodeCache::InodeCache(size_t size, const std::shared_ptr<KVStore> store)
    : max_size_(size), store_(store) {}
std::size_t InodeCacheHash::operator()(const kvfsInodeKey &x) const {
  std::size_t seed = 0;
  seed ^= x.inode_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  seed ^= x.hash_ + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  return seed;
}
}  // namespace kvfs