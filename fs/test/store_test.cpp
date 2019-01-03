/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_test.cpp
 */

#include "../rocksdb/rocksdb_store.hpp"
#include "../rocksdb/rocksdb_hash.hpp"
#include "../store/store.hpp"
#include <stdio.h>
#include <iostream>
#include <random>

using namespace kvfs;

int main() {
    const char             *name = "/tmp/db/";
    std::unique_ptr<Store> store_;

    store_       = std::make_unique<RocksDBStore>(name);

    dir_key root{};
    auto    seed = static_cast<uint32_t>(std::rand());
    root.inode = 0;
    root.hash  = kvfs::XXH32(name, static_cast<int>(strlen(name)), seed);

    std::cout << root.hash << std::endl;

    dir_value root_value = dir_value{};

    root_value.name = "/tmp/db/";

    std::cout << root_value.name << std::endl;
    std::cout << root_value.to_slice().size() << std::endl;

    const kvfs::Slice root_slice       = kvfs::Slice("root");
    const kvfs::Slice root_value_slice = kvfs::Slice("/tmp/");

    bool status = store_->put2(root_slice, root_value_slice);

    if (status) {
        std::cout << "root insert success." << std::endl;

//        string retrieve = db.get2(root_slice);
        StoreResult retrieve = store_->get(root_slice);
        retrieve.ensureValid();
        if (retrieve.isValid()) {
            std::cout << "root retrieve success." << std::endl;
            std::cout << retrieve.extractValue() << std::endl;
            const auto *back = reinterpret_cast<const dir_value *>(retrieve.extractValue().data());
            std::cout << back->name << std::endl;
        }
    }

    if (!status) {
        std::cout << "ERROR" << std::endl;
    }

    store_->close();

    store_.reset();
    return 0;
}
