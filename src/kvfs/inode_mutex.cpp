/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_mutex.cpp
 */


#include <kvfs/inode_mutex.hpp>
#include <folly/Hash.h>
#include <folly/logging/xlog.h>

namespace kvfs {

    RWLock::RWLock() { pthread_rwlock_init(&rw_, nullptr); }

    RWLock::~RWLock() { pthread_rwlock_destroy(&rw_); }

    void RWLock::ReadLock() { pthread_rwlock_rdlock(&rw_); }

    void RWLock::WriteLock() { pthread_rwlock_wrlock(&rw_); }

    void RWLock::Unlock() { pthread_rwlock_unlock(&rw_); }

    void InodeMutex::ReadLock(const _meta_key_t &key) {
        kvfs_inode_t lock_id = (key.inode_id + key.hash_id) & ILOCK_BASE;
//  uint32_t lock_id = leveldb::Hash(key.str, 16, 0) & ILOCK_BASE;
//  ilock[lock_id].ReadLock();
//  Logging::Default()->LogMsg("ReadLock [%d, %d]\n", key.inode_id, key.hash_id);
    }

    void InodeMutex::WriteLock(const _meta_key_t &key) {
        kvfs_inode_t lock_id = (key.inode_id + key.hash_id) & ILOCK_BASE;
//  uint32_t lock_id = leveldb::Hash(key.str, 16, 0) & ILOCK_BASE;
//  ilock[lock_id].WriteLock();
//  Logging::Default()->LogMsg("WriteLock [%d, %d]\n", key.inode_id, key.hash_id);
    }

    void InodeMutex::Unlock(const _meta_key_t &key) {
        kvfs_inode_t lock_id = (key.inode_id + key.hash_id) & ILOCK_BASE;
//  uint32_t lock_id = leveldb::Hash(key.str, 16, 0) & ILOCK_BASE;
//  ilock[lock_id].Unlock();
//  Logging::Default()->LogMsg("Unlock [%d, %d]\n", key.inode_id, key.hash_id);
    }

} // namespace tablefs
