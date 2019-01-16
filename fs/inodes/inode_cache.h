/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_cache.h
 */

#ifndef KVFS_INODE_CACHE_H
#define KVFS_INODE_CACHE_H

#include <store/store.hpp>
#include <store/slice.hpp>
#include <store/store_entry.hpp>
#include <memory>
#include <string>

namespace kvfs {

enum inode_access_mode {
  INODE_READ = 0,
  INODE_DELETE = 1,
  INODE_WRITE = 2,
};

enum inode_state {
  CLEAN = 0,
  DELETED = 1,
  DIRTY = 2,
};

struct inode_cache_handle {
  slice key_;
  slice value_;
  inode_access_mode mode_;

  void *pointer;
  static int object_count;

  inode_cache_handle() :
      mode_(INODE_READ) {
    ++object_count;
  }

  inode_cache_handle(const slice &key,
                     const slice &value,
                     inode_access_mode mode) :
      key_(key), value_(value), mode_(mode) {
    ++object_count;
  }

  ~inode_cache_handle() {
    --object_count;
  }
};

class inode_cache {
 public:

  virtual inode_cache_handle *insert(const slice &key,
                                     const slice &value) = 0;

  virtual inode_cache_handle *get(const slice &key,
                                  const inode_access_mode &mode) = 0;

  virtual void release(inode_cache_handle *handle) = 0;

  virtual void write_back(inode_cache_handle *handle) = 0;

  virtual void batch_commit(inode_cache_handle *handle1,
                            inode_cache_handle *handle2) = 0;

  virtual void evict(const slice &key) = 0;

};

}  // namespace kvfs

#endif //KVFS_INODE_CACHE_H
