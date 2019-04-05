/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_result.cpp
 */


#include "kvfs_store_result.h"

kvfs::KVStoreResult::KVStoreResult(std::string &&data)
    : valid_(true), data_(std::move(data)) {}

bool kvfs::KVStoreResult::isValid() const {
  return valid_;
}
const std::string &kvfs::KVStoreResult::asString() const {
  ensureValid();
  return data_;
}
void kvfs::KVStoreResult::ensureValid() const {
  if (!isValid()) {
    throw FSError(FSErrorType::FS_ENOENT, "The value was not found in the backing store.");
  }
}