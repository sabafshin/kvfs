/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   RocksDBHandles.cpp
 */


#include <store/RocksDBHandles.hpp>

using folly::StringPiece;
using rocksdb::DB;
using rocksdb::Options;
using rocksdb::ReadOptions;
using rocksdb::Status;
using std::string;
using std::unique_ptr;


namespace kvfs {
    namespace store {

        RocksHandles::~RocksHandles() {
            db.reset();
        }

        RocksHandles::RocksHandles(StringPiece dbPath) {
            auto dbPathStr = dbPath.str();

            Options options;
            // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
            options.IncreaseParallelism();

            // Create the DB if it's not already present.
            options.create_if_missing = true;

            DB *dbRaw;

            auto status =
                         DB::Open(options, dbPathStr, &dbRaw);
            if (!status.ok()) {
                throw std::runtime_error(folly::to<string>(
                        "Failed to open DB: ", dbPathStr, ": ", status.ToString()));
            }

            db.reset(dbRaw);
        }

    }
}