/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_cache.cpp
 */

#include "inode_cache.h"

namespace kvfs {
void inode_cache::insert(const data_key &key,
                         const std::string &value) {
  MutexLock lock(cache_mutex);

  auto handle = inode_cache_handle(key, value, INODE_WRITE);
  Entry entry(key, handle);
  cache.push_front(entry);
  lookup[key] = cache.begin();
  if (cache.size() > max_size) {
    auto handle = cache.back().second;
    clean_inode_handle(handle);
    lookup.erase(cache.back().first);
    cache.pop_back();
  }
}
void inode_cache::insert(const dir_key &key, const std::string &value) {
  MutexLock lock(cache_mutex);

  auto handle = inode_cache_handle(key, value, INODE_WRITE);
  Entry entry(data_key{key.inode, key.hash, 0}, handle);
  cache.push_front(entry);
  lookup[entry.first] = cache.begin();
  if (cache.size() > max_size) {
    auto handle = cache.back().second;
    clean_inode_handle(handle);
    lookup.erase(cache.back().first);
    cache.pop_back();
  }
}
bool inode_cache::get(const data_key &key, inode_access_mode mode, inode_cache_handle &handle) {
  MutexLock lock(cache_mutex);

  auto it = lookup.find(key);
  if (it == lookup.end()) {
    StoreResult sr = store_->get(key.to_string());
    if (sr.isValid()) {
      handle = inode_cache_handle(key, sr.asString(), mode);
      this->insert(key, sr.asString());
      return true;
    }
  }
  handle = it->second->second;
  cache.push_front(*(it->second));
  cache.erase(it->second);
  lookup[key] = cache.begin();

  return true;
}
bool inode_cache::get(const dir_key &key, inode_access_mode mode, inode_cache_handle &handle) {
  MutexLock lock(cache_mutex);

  auto it = lookup.find(data_key{key.inode, key.hash});
  if (it == lookup.end()) {
    StoreResult sr = store_->get(key.to_string());
    if (sr.isValid()) {
      handle = inode_cache_handle(key, sr.asString(), mode);
      this->insert(key, sr.asString());
      return true;
    }
  }
  handle = it->second->second;
  cache.push_front(*(it->second));
  cache.erase(it->second);
  lookup[it->second->first] = cache.begin();

  return true;
}

void inode_cache::clean_inode_handle(inode_cache_handle handle) {
  Mutex *store_mutex;

  MutexLock lock(store_mutex);
  if (handle.access_mode == INODE_WRITE) {
    if (handle.isDir) {
      dir_key dk{handle.inode, handle.hash};
      store_->put(dk.to_string(), handle.value);
    } else {
      data_key dk{handle.inode, handle.hash, handle.blockN};
      store_->put(dk.to_string(), handle.value);
    }
  }
  if (handle.access_mode == INODE_DELETE) {
    if (handle.isDir) {
      dir_key dk{handle.inode, handle.hash};
      store_->put(dk.to_string(), handle.value);
    } else {
      data_key dk{handle.inode, handle.hash, handle.blockN};
      store_->put(dk.to_string(), handle.value);
    }
  }

  delete store_mutex;
}

void inode_cache::evict(const data_key &key) {
  MutexLock l(cache_mutex);

  auto it = lookup.find(key);
  if (it != lookup.end()) {
    auto handle = it->second->second;
    clean_inode_handle(handle);
    lookup.erase(it);
  }
}
void inode_cache::evict(const dir_key &key) {
  MutexLock l(cache_mutex);

  auto it = lookup.find(data_key{key.inode, key.hash, 0});
  if (it != lookup.end()) {
    auto handle = it->second->second;
    clean_inode_handle(handle);
    lookup.erase(it);
  }
}
void inode_cache::write_back(inode_cache_handle &handle) {
  MutexLock lock(cache_mutex);

  if (handle.isDir) {
    if (handle.access_mode == INODE_WRITE) {
      dir_key dk{handle.inode, handle.hash};
      store_->put(dk.to_string(), handle.value);
      handle.access_mode = INODE_READ;
    } else if (handle.access_mode == INODE_DELETE) {
      dir_key dk{handle.inode, handle.hash};
      store_->put(dk.to_string(), handle.value);
      this->evict(dk);
    }
  } else {
    if (handle.access_mode == INODE_WRITE) {
      data_key dk{handle.inode, handle.hash, handle.blockN};
      store_->put(dk.to_string(), handle.value);
      handle.access_mode = INODE_READ;
    } else if (handle.access_mode == INODE_DELETE) {
      data_key dk{handle.inode, handle.hash, handle.blockN};
      store_->put(dk.to_string(), handle.value);
      this->evict(dk);
    }
  }
}
size_t inode_cache::size() {
  MutexLock lock(cache_mutex);
  return cache.size();
}
}  // namespace kvfs