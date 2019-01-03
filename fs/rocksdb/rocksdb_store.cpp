/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.cpp
 */

#include "rocksdb_store.hpp"
#include <iostream>

namespace kvfs {
    RocksDBStore::RocksDBStore(string _db_path) : db_handle(std::move(_db_path)) {}

    RocksDBStore::~RocksDBStore() {
        close();
    };

    void RocksDBStore::close() {
        db_handle.db->Close();
        db_handle.db.reset();
    }

    bool RocksDBStore::put(data_key key, slice value) {
        auto status = db_handle.db->Put(rocksdb::WriteOptions(), key.to_slice().value, value.value);

        return status.ok();
    }

    bool RocksDBStore::put(dir_key key, dir_value value) {
        auto status = db_handle.db->Put(WriteOptions(), key.to_slice().value, value.to_slice().value);

        return status.ok();
    }

    StoreResult RocksDBStore::get(slice key) {
        string value;
        auto   status = db_handle.db->Get(
                ReadOptions(), key.value, &value);
        if (!status.ok()) {
            if (status.IsNotFound()) {
                // Return an empty StoreResult
                return StoreResult();
            }

            /*throw RocksException::build(
                    status, "failed to get " + key.data() + " from local store");*/
        }
        return StoreResult(std::move(value));
    }

    bool RocksDBStore::delete_(slice key) {
        auto status = db_handle.db->Delete(WriteOptions(), key.value);

        return status.ok();
    }

    vector<StoreResult> RocksDBStore::get_children(dir_key key) {

        auto val = get(key.to_slice());
        if (val.isValid()){
            const auto *dirValue = reinterpret_cast<const dir_value *>(val.asString().data());
            kvfs_file_inode_t prefix = dirValue->this_inode;
            vector<StoreResult> result;
            auto iter = db_handle.db->NewIterator(ReadOptions());

            for(iter->Seek(reinterpret_cast<const char *>(prefix));
                    iter->Valid() && iter->key().starts_with(reinterpret_cast<const char *>(prefix));
                        iter->Next() )
            {
                auto value = iter->value();
                result.emplace_back(StoreResult(value.data()));
            }

            return result;
        }

        return vector<StoreResult>();
    }

    bool RocksDBStore::get_parent(dir_key key) {
        auto val = get(key.to_slice());

        if (val.isValid()) {
            const auto *dirValue = reinterpret_cast<const dir_value *>(val.asString().data());
            kvfs_file_inode_t inode = dirValue->this_inode-1;

            if (inode < 0){
                return false;
            }

            if (inode >= 0){
                kvfs_file_hash_t hash = dirValue->parent_name;

                dir_key parent_key;
            }

        }
        return false;
    }

    bool RocksDBStore::hasKey(slice key) const {
        string value;
        auto   status = db_handle.db->Get(
                ReadOptions(), key.value, &value);
        if (status.ok()) {
            return true;
            /*throw RocksException::build(
                    status, "failed to get ", key, " from local store");*/
        }
        return false;
    }

    bool RocksDBStore::sync() {
        WriteOptions writeOptions;
        writeOptions.sync = true;
        rocksdb::Status status = db_handle.db->Put(writeOptions, "sync", "");
        return status.ok();
    }

    bool RocksDBStore::compact() {
        auto status = db_handle.db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);

        return status.ok();
    }

    bool RocksDBStore::get(const slice& key, string* value) {
        auto status = db_handle.db->Get(ReadOptions(), key.value, value);
        assert(status.ok());
        assert(!status.IsNotFound());
        return status.ok();
    }

    string RocksDBStore::get2(const slice &key) {
        string value;

        if (db_handle.db->Get(ReadOptions(), key.value, &value).ok())
            return value;
        return "";
    }

    bool RocksDBStore::put2(const slice &key, const slice &value) {
        auto status = db_handle.db->Put(WriteOptions(), key.value, value.value);
        assert(status.ok());
        return status.ok();
    }

}