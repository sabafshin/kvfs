/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_cache.cpp
 */

#include <kvfs/inode_cache.hpp>
#include <rocksdb/write_batch.h>
#include <folly/logging/xlog.h>

namespace kvfs {

    RocksDbStore* InodeCache::metadb_ = nullptr;
    int InodeCacheHandle::object_count = 0;

    rocksdb::Slice _createSlice(const ByteRange &bytes) {
        return Slice(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    }
    void CleanInodeHandle(const rocksdb::Slice &key, void* value) {
        auto * handle = reinterpret_cast<InodeCacheHandle*>(value);
        if (handle->mode_ == INODE_WRITE) {
            InodeCache::metadb_->put(handle->key_.ToByteRange(), handle->value_);
        } else if (handle->mode_ == INODE_DELETE) {
            InodeCache::metadb_->Delete(handle->key_.ToByteRange());
        }

        delete handle;
    }

    InodeCacheHandle* InodeCache::Insert(const _meta_key_t key,
                                         const folly::StringPiece &value) {
        auto * handle = new InodeCacheHandle(key, value, INODE_WRITE);
        rocksdb::Cache::Handle* ch = cache->Lookup(key.ToSlice());
        rocksdb::Status s = cache->Insert(key.ToSlice(), (void *) ch, 1, CleanInodeHandle);
        if (s.ok()) {
            handle->pointer = ch;
            return handle;
        }
        else
//            XLOG(CRITICAL, "Failed to insert InodeCache");

        return nullptr;
    }

    InodeCacheHandle* InodeCache::Get(const _meta_key_t key,
                                      const InodeAccessMode mode) {
        rocksdb::Cache::Handle* ch = cache->Lookup(key.ToSlice());
        InodeCacheHandle* handle = nullptr;
        if (ch == nullptr) {
            StoreResult value = metadb_->get(key.ToByteRange());
            if (value.isValid()) {
                handle = new InodeCacheHandle(key, value.piece(), mode);
                rocksdb::Status s = cache->Insert(key.ToSlice(), ch, 1, CleanInodeHandle);
                handle->pointer = ch;
            }
        } else {
            handle = (InodeCacheHandle*) cache->Value(ch);
            if (mode > INODE_READ) {
                handle->mode_ = mode;
            }
        }
        return handle;
    }

    void InodeCache::Release(InodeCacheHandle* handle) {
        cache->Release(handle->pointer);
    }

    void InodeCache::WriteBack(InodeCacheHandle* handle) {
#ifdef KVFS_DEBUG
        if (Logging::Default() != NULL) {
    const tfs_inode_header* header = reinterpret_cast<const tfs_inode_header*> (handle->value_.data());
    Logging::Default()->LogMsg("Write Back: inode_id(%08x) hash_id(%08x) mode_(%d) size(%d)\n",
                                handle->key_.inode_id,
                                handle->key_.hash_id,
                                handle->mode_,
                                header->fstat.st_size);
  }
#endif

        if (handle->mode_ == INODE_WRITE) {
            InodeCache::metadb_->put(handle->key_.ToByteRange(), handle->value_);
            handle->mode_ = INODE_READ;
        } else if (handle->mode_ == INODE_DELETE) {
            InodeCache::metadb_->Delete(handle->key_.ToByteRange());
            Evict(handle->key_);
        }
    }

    void InodeCache::BatchCommit(InodeCacheHandle* handle1,
                                 InodeCacheHandle* handle2) {
#ifdef KVFS_DEBUG
        if (Logging::Default() != NULL) {
    const tfs_inode_header* header = reinterpret_cast<const tfs_inode_header*> (handle1->value_.data());
    Logging::Default()->LogMsg("Batch commit [write back]: inode_id(%08x) hash_id(%08x) mode_(%d) size(%d)\n",
                                handle1->key_.inode_id,
                                handle1->key_.hash_id,
                                handle1->mode_,
                                header->fstat.st_size);
    header = reinterpret_cast<const tfs_inode_header*> (handle2->value_.data());
    Logging::Default()->LogMsg("Batch commit [write back]: inode_id(%08x) hash_id(%08x) mode_(%d) size(%d)\n",
                                handle2->key_.inode_id,
                                handle2->key_.hash_id,
                                handle2->mode_,
                                header->fstat.st_size);

  }
#endif

        rocksdb::WriteBatch batch;
        batch.Clear();
        if (handle1->mode_ == INODE_WRITE) {
            batch.Put(_createSlice(handle1->key_.ToByteRange()), _createSlice(handle1->value_));
            handle1->mode_ = INODE_READ;
        } else if (handle1->mode_ == INODE_DELETE) {
            batch.Delete(handle1->key_.ToSlice());
            Evict(handle1->key_);
        }
        if (handle2->mode_ == INODE_WRITE) {
            batch.Put(_createSlice(handle2->key_.ToByteRange()), _createSlice(handle2->value_));
            handle2->mode_ = INODE_READ;
        } else if (handle2->mode_ == INODE_DELETE) {
            batch.Delete(handle2->key_.ToSlice());
            Evict(handle2->key_);
        }
        metadb_->beginWrite(batch);
    }

    void InodeCache::Evict(const _meta_key_t key) {
        cache->Erase(key.ToSlice());
    }

} // namespace kvfs
