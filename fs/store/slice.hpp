/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_slice.hpp
 */

#ifndef KVFS_SLICE_HPP
#define KVFS_SLICE_HPP

#include <string>
#ifdef KVFS_ROCKSDB_SLICE_HPP
#include "../kvfs_rocksdb/rocksdb_slice.hpp"
#endif

namespace kvfs {

class slice : public rocksdb_slice {
 public:
  // Create an empty store_slice.
  slice() : rocksdb_slice() {};

  // Create a store_slice that refers to d[0,n-1].
  slice(const char *d, size_t n) : rocksdb_slice(d, n) {};

  // Create a store_slice that refers to the contents of "s"
  /* implicit */
  slice(const std::string &s) : rocksdb_slice(s) {};

  // Create a store_slice that refers to s[0,strlen(s)-1]
  /* implicit */
  slice(const char *s) : rocksdb_slice(s) {};

};
}

#endif //KVFS_STORE_SLICE_HPP
