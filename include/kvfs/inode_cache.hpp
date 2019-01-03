#include <utility>

/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_cache.hpp
 */

#ifndef KVFS_INODE_CACHE_HPP
#define KVFS_INODE_CACHE_HPP

#include <store/RocksDBStore.hpp>
#include <kvfs/inode_mutex.hpp>
#include <kvfs/inode.hpp>
#include <rocksdb/cache.h>
#include <port/port.h>

using namespace kvfs::store;

namespace kvfs {

    enum InodeAccessMode {
        INODE_READ = 0,
        INODE_DELETE = 1,
        INODE_WRITE = 2,
    };

    enum InodeState {
        CLEAN = 0,
        DELETED = 1,
        DIRTY = 2,
    };

    struct InodeCacheHandle {
        _meta_key_t key_;
        folly::StringPiece value_;
        InodeAccessMode mode_;

        rocksdb::Cache::Handle* pointer;
        static int object_count;

        InodeCacheHandle():
                mode_(INODE_READ) {
            ++object_count;
        }

        InodeCacheHandle(const _meta_key_t &key,
                         folly::StringPiece value,
                         InodeAccessMode mode):
                key_(key), value_(std::move(value)), mode_(mode) {
            ++object_count;
        }

        ~InodeCacheHandle() {
            --object_count;
        }
    };

    class InodeCache {
    public:
        static RocksDbStore* metadb_;
        std::shared_ptr<rocksdb::Cache> cache;

        explicit InodeCache(RocksDbStore *metadb) {
            cache = rocksdb::NewLRUCache(MAX_OPEN_FILES);
            metadb_ = metadb;
        }

        ~InodeCache() {
            cache.reset();
        }

        InodeCacheHandle* Insert(const _meta_key_t key,
                                 const folly::StringPiece &value);

        InodeCacheHandle* Get(const _meta_key_t key,
                              const InodeAccessMode mode);

        void Release(InodeCacheHandle* handle);

        void WriteBack(InodeCacheHandle* handle);

        void BatchCommit(InodeCacheHandle* handle1,
                         InodeCacheHandle* handle2);

        void Evict(const _meta_key_t key);

        friend void CleanInodeHandle(const rocksdb::Slice key, void* value);

    };

}

#endif //KVFS_INODE_CACHE_HPP
