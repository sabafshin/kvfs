/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_slice.hpp
 */

#ifndef KVFS_ROCKSDB_SLICE_HPP
#define KVFS_ROCKSDB_SLICE_HPP

#include <string>
#include <rocksdb/slice.h>

namespace kvfs {

class rocksdb_slice : public rocksdb::Slice {
 protected:
  rocksdb_slice(const char *d, size_t n);
  rocksdb_slice();
  rocksdb_slice(const std::string &s);
  rocksdb_slice(const char *s);
};
}
#endif //KVFS_ROCKSDB_SLICE_HPP