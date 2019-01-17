/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   directory_entry.cpp
 */

#include "directory_entry.h"

bool kvfs::dentry_cache::find(kvfs::dir_key &key, kvfs::dir_value &value) {
  MutexLock l(d_cache_mutex);

  auto it = lookup.find(key);
  if (it == lookup.end()) {
    return false;
  } else {

    value = it->second->second;
    cache.push_front(*(it->second));
    cache.erase(it->second);
    lookup[key] = cache.begin();

    return true;
  }
}
void kvfs::dentry_cache::insert(kvfs::dir_key &key, kvfs::dir_value &value) {
  MutexLock l(d_cache_mutex);

  Entry ent(key, value);
  cache.push_front(ent);
  lookup[key] = cache.begin();
  if (cache.size() > maxsize) {
    lookup.erase(cache.back().first);
    cache.pop_back();
  }

}
void kvfs::dentry_cache::evict(kvfs::dir_key &key) {
  MutexLock l(d_cache_mutex);

  auto it = lookup.find(key);
  if (it != lookup.end()) {
    lookup.erase(it);
  }
}