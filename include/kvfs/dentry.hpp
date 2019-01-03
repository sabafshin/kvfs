/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   dentry.hpp
 */

#ifndef KVFS_DIR_ENTRY_CACHE_HPP
#define KVFS_DIR_ENTRY_CACHE_HPP

#include <kvfs/inode.hpp>
#include <folly/Hash.h>
#include <unordered_map>
#include <list>
#include <port/port_posix.h>
#include <util/mutexlock.h>

using std::unordered_map;

namespace kvfs {

    struct DentryCacheComp {
        bool operator()(const _meta_key_t &lhs, const _meta_key_t &rhs) const {
            return (lhs.hash_id == rhs.hash_id) && (lhs.inode_id == rhs.inode_id);
        }
    };

    struct DentryCacheHash {
        std::size_t operator()(const _meta_key_t &x) const {
            std::size_t seed = 0;
            seed ^= x.inode_id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= x.hash_id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            return seed;
        }
    };

    class DentryCache {
    public:
        typedef std::pair<_meta_key_t, kvfs_inode_t> Entry;
        typedef std::list<Entry> CacheList;
        typedef unordered_map <_meta_key_t, std::list<Entry>::iterator,
                DentryCacheHash, DentryCacheComp> CacheMap;

        explicit DentryCache(size_t size) : maxsize(size) {}

        bool Find(_meta_key_t &key, kvfs_inode_t &value);

        void Insert(_meta_key_t &key, const kvfs_inode_t &value);

        void Evict(_meta_key_t &key);

        void GetSize(size_t &size_cache_list, size_t &size_cache_map) {
            size_cache_map = cache.size();
            size_cache_list = lookup.size();
        }

        ~DentryCache() = default;

    private:
        size_t maxsize;
        CacheList cache;
        CacheMap lookup;

        rocksdb::port::Mutex cache_mutex;
    };

} //namespace kvfs

#endif //KVFS_DIR_ENTRY_CACHE_HPP
