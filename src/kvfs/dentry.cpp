/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   dentry.cpp
 */


#include <kvfs/dentry.hpp>

using rocksdb::MutexLock;

namespace kvfs {

    bool DentryCache::Find(_meta_key_t &key, kvfs_inode_t &value) {
        MutexLock lock_cache_mutex(&cache_mutex);

        auto it = lookup.find(key);
        if (it == lookup.end()) {
            return false;
        }
        else {
            value = it->second->second;
            cache.push_front(*(it->second));
            cache.erase(it->second);
            lookup[key] = cache.begin();
            return true;
        }
    }

    void DentryCache::Insert(_meta_key_t &key, const kvfs_inode_t &value) {
        MutexLock lock_cache_mutex(&cache_mutex);

        Entry ent(key, value);
        cache.push_front(ent);
        lookup[key] = cache.begin();
        if (cache.size() > maxsize) {
            lookup.erase(cache.back().first);
            cache.pop_back();
        }
    }

    void DentryCache::Evict(_meta_key_t &key) {
        MutexLock lock_cache_mutex(&cache_mutex);

        auto it = lookup.find(key);
        if (it != lookup.end()) {
            lookup.erase(it);
        }
    }

}