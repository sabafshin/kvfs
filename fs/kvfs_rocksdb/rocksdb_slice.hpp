/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_slice.hpp
 */

#ifndef KVFS_SLICE_HPP
#define KVFS_SLICE_HPP

#include <string>
#include <cstring>
#include <assert.h>
#include <rocksdb/slice.h>

namespace kvfs {
struct rocksdb_slice {
        rocksdb::Slice value;

  ~rocksdb_slice();

  explicit rocksdb_slice(const std::string &s);
  explicit rocksdb_slice(const char *d, size_t n);
  explicit rocksdb_slice(std::string_view sv);
  explicit rocksdb_slice(const char *s);

  rocksdb_slice(const rocksdb_slice &) = default;

  rocksdb_slice &operator=(const rocksdb_slice &) = delete;

  rocksdb_slice(rocksdb_slice &&) = default;

  rocksdb_slice &operator=(rocksdb_slice &&) = default;
    };
}

#endif //KVFS_SLICE_HPP
