/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocks_db_handler.cpp
 */

#include "rocks_db_handler.hpp"

namespace kvfs {

    RocksHandles::~RocksHandles(){
        db.reset();
    }

    RocksHandles::RocksHandles(string dbPath) {

        Options options;
        // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
        options.IncreaseParallelism();

        // Create the DB if it's not already present.
        options.create_if_missing = true;

        DB *dbRaw;

        auto status =
                     DB::Open(options, dbPath, &dbRaw);
        if (!status.ok()) {
            throw std::runtime_error("Failed to open DB at the given path");
        }

        db.reset(dbRaw);
    }

    RocksIterator::~RocksIterator() {
        it.reset();
    }

    RocksIterator::RocksIterator(RocksHandles db) {
        it.reset(db.db->NewIterator(ReadOptions()));
    }
}
