/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   test.cpp
 */

#include <rocksdb/db.h>
#include <cstdio>
#include <string>
#include <cassert>
#include <iostream>

using namespace rocksdb;

struct key {
    uint64_t  inode;
    uint32_t  hash;

    Slice to_slice(){
        return Slice((const char *) this, sizeof(key));
    }
};

struct value {
    std::string name;
    uint32_t parent_hash;
    Slice to_slice(){
        return Slice((const char *) this, sizeof(key));
    }
};

int main() {
    DB *db;
    Options options;
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
//    options.IncreaseParallelism();
//    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;

    std::string kDBPath = "/tmp/db/";
    // open DB
    Status      s       = DB::Open(options, kDBPath, &db);
    assert(s.ok());

    key root_key = key{
        0,
        0
    };

    value root_value = value{
        "/tmp/db",
        0
    };
    // Put key-value
    s = db->Put(WriteOptions(), root_key.to_slice(), root_value.to_slice());
    assert(s.ok());
    // get value
    std::string value_str;
    s = db->Get(ReadOptions(), root_key.to_slice(), &value_str);
    assert(s.ok());
//    assert(value == "/tmp/db/");
    const auto *res = reinterpret_cast<const value *>(value_str.data());

    std::cout << res->name << std::endl;

    delete db;

    return 0;
}
