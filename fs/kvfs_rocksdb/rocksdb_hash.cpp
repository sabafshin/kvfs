/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_hash.cpp
 */

#include "rocksdb_hash.h"

namespace kvfs {
unsigned int XXH32(const void *input, int len, unsigned int seed) {
  return rocksdb::XXH32(input, len, seed);
}

void *XXH32_init(unsigned int seed) {
  return rocksdb::XXH32_init(seed);
}

XXH_errorcode XXH32_update(void *state, const void *input, int len) {
  return rocksdb::XXH32_update(state, input, len);
}

unsigned int XXH32_digest(void *state) {
  return rocksdb::XXH32_digest(state);
}

int XXH32_sizeofState() {
  return rocksdb::XXH32_sizeofState();
}

XXH_errorcode XXH32_resetState(void *state, unsigned int seed) {
  return rocksdb::XXH32_resetState(state, seed);
}

unsigned int XXH32_intermediateDigest(void *state) {
  return rocksdb::XXH32_intermediateDigest(state);
}
}  // namespace kvfs










