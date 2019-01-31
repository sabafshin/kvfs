/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_hash.h
 */

#ifndef KVFS_STORE_HASH_H
#define KVFS_STORE_HASH_H

#include <kvfs_rocksdb/rocksdb_hash.h>

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
}  // namespace kvfs

#endif //KVFS_STORE_HASH_H
