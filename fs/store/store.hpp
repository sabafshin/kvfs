/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store.hpp
 */

#ifndef KVFS_STORE_HPP
#define KVFS_STORE_HPP

#include <memory>
#include <optional>
#include <vector>

#include "../rocksdb/slice.hpp"
#include "store_entry.hpp"
#include "store_result.hpp"

using namespace std;

namespace kvfs {

    class Store {
    public:
        virtual bool put(data_key key, slice value) = 0;
        virtual bool put(dir_key key, dir_value value) = 0;
        virtual bool put2(const slice& key, const slice& value) = 0;

        virtual StoreResult get(slice key) = 0;
        virtual bool get(const slice &key, string *value) = 0;
        virtual string get2(const slice &key) = 0;

        virtual bool delete_(slice key) = 0;

        virtual bool compact() = 0;

        virtual vector<StoreResult> get_children(dir_key key) = 0;

        virtual bool get_parent(dir_key key) = 0;

        virtual bool sync() = 0;

        virtual void close() = 0;

        virtual bool hasKey(slice key) const = 0;

    };
}

#endif //KVFS_STORE_HPP
