/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_hash.hpp
 */

#ifndef KVFS_ROCKSDB_ROCKSDB_HASH_HPP
#define KVFS_ROCKSDB_ROCKSDB_HASH_HPP

#include <rocksdb/util/xxhash.h>

using namespace rocksdb;

namespace kvfs {
// Simple Hash functions
    unsigned int XXH32(const void *input, int len, unsigned int seed);

// Advanced Hash functions
    void *XXH32_init(unsigned int seed);

    XXH_errorcode XXH32_update(void *state, const void *input, int len);

    unsigned int XXH32_digest(void *state);

    int XXH32_sizeofState();

    XXH_errorcode XXH32_resetState(void *state, unsigned int seed);

    unsigned int XXH32_intermediateDigest(void *state);
}
#endif //KVFS_ROCKSDB_ROCKSDB_HASH_HPP
