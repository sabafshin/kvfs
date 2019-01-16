/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_cache.cpp
 */

#include "rocksdb_cache.h"

namespace kvfs {
rocksdb_cache::rocksdb_cache(const std::shared_ptr<Store> &db) {
  store = db;
  inode_cache_handle::object_count = 0;
}

rocksdb_cache::~rocksdb_cache() {
  store.reset();
  rc.cache.reset();
}

void clean_inode_handle(const rocksdb::Slice &key, void *value) {
  auto *handle = reinterpret_cast<const inode_cache_handle *>(value);
  if (handle->mode_ == INODE_WRITE) {
    rocksdb_cache::store->put(handle->key_, handle->value_);
  } else if (handle->mode_ == INODE_DELETE) {
    rocksdb_cache::store->delete_(handle->key_);
  }
  delete handle;
}

inode_cache_handle *rocksdb_cache::insert(const slice &key, const slice &value) {
  auto *handle = new inode_cache_handle(key, value, INODE_WRITE);
  rocksdb::Cache::Handle *ch = rc.cache->Lookup(key);
  auto s = rc.cache->Insert(key, (void *) ch, 1, clean_inode_handle);
  if (s.ok()) {
    handle->pointer = ch;
    return handle;
  }
  return nullptr;
}
inode_cache_handle *rocksdb_cache::get(const slice &key, const inode_access_mode &mode) {
  rocksdb::Cache::Handle *ch = rc.cache->Lookup(key);
  inode_cache_handle *handle = nullptr;
  if (ch == nullptr) {
    StoreResult value = store->get(key);
    if (value.isValid()) {
      handle = new inode_cache_handle(key, slice(value.asString()), mode);
      auto s = rc.cache->Insert(key, ch, 1, clean_inode_handle);
      handle->pointer = ch;
    }

  } else {
    handle = static_cast<inode_cache_handle *>(rc.cache->Value(ch));
    if (mode > INODE_READ) {
      handle->mode_ = mode;
    }
  }
  return handle;
}
void rocksdb_cache::release(inode_cache_handle *handle) {
  rc.cache->Release(static_cast<rocksdb::Cache::Handle *>(handle->pointer));
}
void rocksdb_cache::write_back(inode_cache_handle *handle) {
  if (handle->mode_ == INODE_WRITE) {
    store->put(handle->key_, handle->value_);
    handle->mode_ = INODE_READ;
  } else if (handle->mode_ == INODE_DELETE) {
    store->delete_(handle->key_);
    evict(handle->key_);
  }
}

void rocksdb_cache::batch_commit(inode_cache_handle *handle1, inode_cache_handle *handle2) {
  size_t buf_size = 2 * sizeof(inode_cache_handle);
  auto write_batch = store->beginWrite(buf_size);
  write_batch->flush();

  if (handle1->mode_ == INODE_WRITE) {
    write_batch->put(handle1->key_, handle1->value_);
    handle1->mode_ = INODE_READ;
  }
  if (handle1->mode_ == INODE_DELETE) {
    write_batch->delete_(handle1->key_);
    evict(handle1->key_);
  }
  if (handle2->mode_ == INODE_WRITE) {
    write_batch->put(handle2->key_, handle2->value_);
    handle2->mode_ = INODE_READ;
  }
  if (handle2->mode_ == INODE_DELETE) {
    write_batch->delete_(handle2->key_);
    evict(handle2->key_);
  }
  write_batch->flush();
}
void rocksdb_cache::evict(const slice &key) {
  rc.cache->Erase(key);
}

}  // namespace kvfs