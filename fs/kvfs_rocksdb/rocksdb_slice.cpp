/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_slice.cpp
 */

#include "rocksdb_slice.hpp"

kvfs::rocksdb_slice::rocksdb_slice(const char *d, size_t n) : Slice(d, n) {}
kvfs::rocksdb_slice::rocksdb_slice() : Slice() {}
kvfs::rocksdb_slice::rocksdb_slice(const std::string &s) : Slice(s) {}
kvfs::rocksdb_slice::rocksdb_slice(const char *s) : Slice(s) {}
