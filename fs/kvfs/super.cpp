/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   super.cpp
 */

#include <kvfs/super.h>

namespace kvfs {

void kvfsSuperBlock::parse(const KVStoreResult &sr) {
  auto bytes_ = sr.asString();
  if (bytes_.size() != sizeof(kvfsSuperBlock)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsSuperBlock";
    oss << " is: (" << sizeof(kvfsSuperBlock) << ") ";
    oss << "but retrieved size: (" << bytes_.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes_.data();
  memmove(this, idx, sizeof(kvfsSuperBlock));
}
std::string kvfsSuperBlock::pack() const {
  std::string d(sizeof(kvfsSuperBlock), L'\0');
  memcpy(&d[0], this, d.size());
  return d;
}
std::string kvfsFreedInodesKey::pack() const {
  std::string d(sizeof(kvfsFreedInodesKey), L'\0');
  memcpy(&d[0], this, d.size());
  return d;
}
std::string kvfsFreedInodesValue::pack() const {
  std::string d(sizeof(kvfsFreedInodesValue), L'\0');
  memcpy(&d[0], this, d.size());
  return d;
}
void kvfsFreedInodesValue::parse(const KVStoreResult &sr) {
  auto bytes_ = sr.asString();
  if (bytes_.size() != sizeof(kvfsFreedInodesValue)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsFreedInodesValue";
    oss << " is: (" << sizeof(kvfsFreedInodesValue) << ") ";
    oss << "but retrieved size: (" << bytes_.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes_.data();
  memcpy(this, idx, sizeof(kvfsFreedInodesValue));
}
}