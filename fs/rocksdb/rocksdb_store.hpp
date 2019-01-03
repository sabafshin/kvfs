/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.hpp
 */

#ifndef KVFS_ROCKSDB_STORE_HPP
#define KVFS_ROCKSDB_STORE_HPP

#include "rocks_db_handler.hpp"
#include "rocks_db_exception.hpp"
#include "slice.hpp"

#include "../store/store.hpp"
#include "../store/store_entry.hpp"

#include <rocksdb/db.h>

namespace kvfs {

    class RocksDBStore : public Store {
    public:
        explicit RocksDBStore(string _db_path);

        ~RocksDBStore();

        void close() override;

        bool put(data_key key, Slice value) override;
        bool put(dir_key key, dir_value value) override;
        bool put2(const Slice& key, const Slice& value) override;

        StoreResult get(Slice key) override;
        bool get(const Slice& key, string* value) override;
        string get2(const Slice& key) override;

        bool delete_(Slice key) override;

        vector<StoreResult> get_children(dir_key key) override;
        bool get_parent(dir_key key) override;

        bool hasKey(Slice key) const override;

        bool sync() override;

        bool compact() override;

    private:
        RocksHandles db_handle;
    };
}
#endif //KVFS_ROCKSDB_STORE_HPP
