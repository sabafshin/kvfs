/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.cpp
 */

#include "directory_entry_cache.h"

bool kvfs::DentryCache::find(kvfs::StoreEntryKey &key, kvfs::StoreEntryValue &value) {
  MutexLock lock;

  auto it = lookup_.find(key);
  if (it == lookup_.end()) {
    return false;
  } else {

    value = it->second->second;
    cache_.push_front(*(it->second));
    cache_.erase(it->second);
    lookup_[key] = cache_.begin();

    return true;
  }
}
void kvfs::DentryCache::insert(kvfs::StoreEntryKey &key, kvfs::StoreEntryValue &value) {
  MutexLock lock;

  Entry ent(key, value);
  cache_.push_front(ent);
  lookup_[key] = cache_.begin();
  if (cache_.size() > maxsize_) {
    lookup_.erase(cache_.back().first);
    cache_.pop_back();
  }

}
void kvfs::DentryCache::evict(kvfs::StoreEntryKey &key) {
  MutexLock lock;
  auto it = lookup_.find(key);
  if (it != lookup_.end()) {
    lookup_.erase(it);
  }
}