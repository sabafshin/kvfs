/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_entry.cpp
 */

#include "store_entry.hpp"

namespace kvfs {
void dir_value::parse(const kvfs::StoreResult &result) {
  auto bytes = result.asString();
  if (bytes.size() != sizeof(dir_value)) {
    throw std::invalid_argument("Bad size");
  }
  auto *idx = bytes.data();
  memmove(this, idx, sizeof(dir_value));
}
}  // namespace kvfs