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

#include "../kvfs_rocksdb/rocksdb_slice.hpp"
#include "store_entry.hpp"
#include "store_result.hpp"

using namespace std;

namespace kvfs {

    class Store {
    public:
      virtual bool put(data_key key, rocksdb_slice value) = 0;
        virtual bool put(dir_key key, dir_value value) = 0;

      virtual bool merge(rocksdb_slice key, rocksdb_slice value) = 0;

      virtual StoreResult get(rocksdb_slice key) = 0;

      virtual bool delete_(rocksdb_slice key) = 0;
      virtual bool delete_range(rocksdb_slice start, rocksdb_slice end) = 0;

        virtual bool compact() = 0;

        virtual vector<StoreResult> get_children(dir_key key) = 0;

        virtual bool get_parent(dir_key key) = 0;

        virtual bool sync() = 0;

        virtual void close() = 0;

      virtual bool hasKey(rocksdb_slice key) const = 0;

      class WriteBatch {
       public:
        virtual void put(data_key key, rocksdb_slice value) = 0;
        virtual void put(dir_key key, dir_value value) = 0;

        virtual void delete_(rocksdb_slice key) = 0;

        /**
        * Flush any pending data to the store.
        */
        virtual void flush() = 0;

        // Forbidden copy construction/assignment; allow only moves
        WriteBatch(const WriteBatch &) = delete;
        WriteBatch(WriteBatch &&) = default;
        WriteBatch &operator=(const WriteBatch &) = delete;
        WriteBatch &operator=(WriteBatch &&) = default;
        virtual ~WriteBatch();
        WriteBatch() = default;

       private:
        friend class Store;
      };

      virtual std::unique_ptr<WriteBatch> beginWrite(size_t buffer_size) = 0;

    };

Store::WriteBatch::~WriteBatch() = default;
}

#endif //KVFS_STORE_HPP
