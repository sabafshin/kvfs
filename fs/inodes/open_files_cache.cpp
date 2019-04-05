/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.cpp
 */

#include "open_files_cache.h"

bool kvfs::OpenFilesCacheComparator::operator()(const int &lhs, const int &rhs) const {
  return (lhs == rhs);
}
std::size_t kvfs::OpenFilesCacheHash::operator()(const int &x) const {
  return static_cast<size_t>(x);
}

kvfs::OpenFilesCache::OpenFilesCache(size_t size)
    : maxsize_(size), mutex_(std::make_unique<std::mutex>()) {}

bool kvfs::OpenFilesCache::Find(const int &filedes, kvfs::kvfsFileHandle &value) {
  mutex_->lock();

  auto it = lookup_.find(filedes);
  if (it == lookup_.end()) {
    mutex_->unlock();
    return false;
  } else {

    value = it->second->second;
    cache_.push_front(*(it->second));
    cache_.erase(it->second);
    lookup_[filedes] = cache_.begin();
    mutex_->unlock();
    return true;
  }
}
void kvfs::OpenFilesCache::Insert(const int &filedes, kvfs::kvfsFileHandle &value) {
  mutex_->lock();

  Entry ent(filedes, value);
  cache_.push_front(ent);
  lookup_[filedes] = cache_.begin();
  // not an lru cache
  /*if (cache_.size() > maxsize_) {
    lookup_.erase(cache_.back().first);
    cache_.pop_back();
  }*/
  mutex_->unlock();
}
void kvfs::OpenFilesCache::Evict(const int &filedes) {
  mutex_->lock();

  auto it = lookup_.find(filedes);
  if (it != lookup_.end()) {
    lookup_.erase(it);
  }

  mutex_->unlock();
}
void kvfs::OpenFilesCache::Size(size_t &size_cache_list, size_t &size_cache_map) {
  size_cache_map = cache_.size();
  size_cache_list = lookup_.size();
}
kvfs::OpenFilesCache::~OpenFilesCache() {
  mutex_.reset();
}