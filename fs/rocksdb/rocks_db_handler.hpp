/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_handler.hpp
 */

#ifndef KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP
#define KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP

#include <rocksdb/db.h>
#include <memory>
#include <string>

using rocksdb::DB;
using rocksdb::Options;
using rocksdb::ReadOptions;
using rocksdb::Status;
using std::string;
using std::unique_ptr;

namespace kvfs {

    /**
     * This class is the holder of the database
     * required to interact with our local rocksdb store.
     */
    struct RocksHandles {
        std::unique_ptr<DB> db;

        ~RocksHandles();

        /**
         * Returns an instance of a RocksDB that uses the specified directory for
         * storage. If there is an existing RocksDB at that path with
         * then it will be opened and
         * returned.  If there is no existing RocksDB at that location a new one will
         * be initialized .
         */
        explicit RocksHandles(string dbPath);

        RocksHandles(const RocksHandles &) = delete;

        RocksHandles &operator=(const RocksHandles &) = delete;

        RocksHandles(RocksHandles &&) = default;

        RocksHandles &operator=(RocksHandles &&) = default;
    };
}


#endif //KVFS_ROCKSDB_ROCKS_DB_HANDLER_HPP
