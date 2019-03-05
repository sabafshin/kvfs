/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.cpp
 */

#include "directory_entry_cache.h"

bool kvfs::DentryCache::find(const int &filedes, kvfs::kvfsFileHandle &value) {
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
void kvfs::DentryCache::insert(const int &filedes, kvfs::kvfsFileHandle &value) {
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
void kvfs::DentryCache::evict(const int &filedes) {
  mutex_->lock();

  auto it = lookup_.find(filedes);
  if (it != lookup_.end()) {
    lookup_.erase(it);
  }

  mutex_->unlock();
}