/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_slice.cpp
 */

#include <memory>
#include "rocksdb_slice.hpp"

using rocksdb::Slice;

kvfs::rocksdb_slice::~rocksdb_slice() = default;

kvfs::rocksdb_slice::rocksdb_slice(const std::string &s) {
    value = Slice(s);
}

kvfs::rocksdb_slice::rocksdb_slice(const char *d, size_t n) {
    value = Slice(d, n);
}

kvfs::rocksdb_slice::rocksdb_slice(std::string_view sv) {
    value = Slice(sv);
}

kvfs::rocksdb_slice::rocksdb_slice(const char *s) {
    value = Slice(s);
}






