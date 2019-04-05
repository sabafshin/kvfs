/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_entry.cpp
 */

#include "kvfs_store_entry.h"
#include <string.h>

namespace kvfs {

void kvfsInodeKey::parse(const std::string &sr) {
  auto bytes = sr.data();
  if (sr.size() != sizeof(kvfsInodeKey)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsInodeKey";
    oss << " is:( " << sizeof(kvfsInodeKey) << ") ";
    oss << "but retrieved size: (" << sr.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes;
  std::memcpy(this, idx, sizeof(kvfsInodeKey));
}
bool kvfsInodeKey::operator==(const kvfsInodeKey &c2) const {
  return c2.inode_ == this->inode_ && c2.hash_ == this->hash_;
}
bool kvfsInodeKey::operator!=(const kvfsInodeKey &c2) const {
  return c2.inode_ != this->inode_ && c2.hash_ != this->hash_;
}
std::string kvfsInodeKey::pack() const {
  std::string d(sizeof(kvfsInodeKey), L'\0');
  std::memcpy(&d[0], this, d.size());
  return d;
}
kvfsBlockKey::kvfsBlockKey(kvfs_file_inode_t inode, kvfs_off_t number) : inode_(inode), block_number_(number) {}
std::string kvfsBlockKey::pack() const {
  std::string d(sizeof(kvfsBlockKey), L'\0');
  std::memcpy(&d[0], this, d.size());
  return d;
}
void kvfsBlockKey::parse(const std::string &sr) {
  auto bytes = sr.data();
  if (sr.size() != sizeof(kvfsBlockKey)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsBlockKey";
    oss << " is:( " << sizeof(kvfsBlockKey) << ") ";
    oss << "but retrieved size: (" << sr.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes;
  std::memcpy(this, idx, sizeof(kvfsBlockKey));
}
bool kvfsBlockKey::operator==(const kvfsBlockKey &c2) const {
  return c2.block_number_ == this->block_number_ && c2.inode_ == this->inode_;
}
const void *kvfsBlockValue::write(const void *buffer, size_t buffer_size) {
  if (size_ != KVFS_DEF_BLOCK_SIZE) {
    std::memcpy(&data[size_], buffer, buffer_size);
    // update size
    size_ += buffer_size;
    // return idx in buffer after write
    auto output = static_cast<const byte *>(buffer) + buffer_size;
    return output;
  }
  return buffer;
}
const void *kvfsBlockValue::write_at(const void *buffer, size_t buffer_size, kvfs_off_t offset) {
  // offset is between 0 and KVFS_BLOCK_SIZE
  size_ = offset + buffer_size;
  std::memcpy(&data[offset], buffer, buffer_size);
  auto output = static_cast<const byte *>(buffer) + buffer_size;
  return output;
}
void *kvfsBlockValue::read(void *buffer, size_t size) const {
  std::memcpy(buffer, data, size);
  void *idx = static_cast<byte *>(buffer) + size;
  return idx;
}
void *kvfsBlockValue::read_at(void *buffer, size_t size, kvfs_off_t offset) const {
  std::memcpy(buffer, &data[offset], size);
  void *idx = static_cast<byte *>(buffer) + size;
  return idx;
}
std::string kvfsBlockValue::pack() const {
  std::string d(sizeof(kvfsBlockValue), L'\0');
  std::memcpy(&d[0], this, d.size());
  return d;
}
void kvfsBlockValue::parse(const KVStoreResult &sr) {
  auto bytes = sr.asString();
  if (bytes.size() != sizeof(kvfsBlockValue)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsBlockValue";
    oss << " is:( " << sizeof(kvfsBlockValue) << ") ";
    oss << "but retrieved size: (" << bytes.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes.data();
  std::memcpy(this, idx, sizeof(kvfsBlockValue));
}
kvfsInodeValue::kvfsInodeValue(const std::string &name,
                               const kvfs_file_inode_t &inode,
                               const mode_t &mode,
                               const kvfsInodeKey &real_key) {
  name.copy(dirent_.d_name, name.length());
  // get a free inode for this new file
  dirent_.d_ino = inode;
  dirent_.d_reclen = name.size();
  fstat_.st_ino = dirent_.d_ino;
  // generate stat
  fstat_.st_mode = mode;
  fstat_.st_blocks = 0;
  fstat_.st_ctim.tv_sec = std::time(nullptr);
  fstat_.st_blksize = KVFS_DEF_BLOCK_SIZE;
  // owner
  fstat_.st_uid = getuid();
  fstat_.st_gid = getgid();
  // link count
//    if (S_ISREG(mode)) {
  fstat_.st_nlink = 1;
  /*} else {
    fstat_.st_nlink = 2;
  }*/
  fstat_.st_blocks = 0;
  fstat_.st_size = 0;
  real_key_ = real_key;
}
void kvfsInodeValue::parse(const kvfs::KVStoreResult &result) {
  auto bytes = result.asString();
  if (bytes.size() != sizeof(kvfsInodeValue)) {
    std::ostringstream oss;
    oss << "Unexpected value size retrieved from the backing store, "
           "expected size for ";
    oss << "kvfsInodeValue";
    oss << " is:( " << sizeof(kvfsInodeValue) << ") ";
    oss << "but retrieved size: (" << bytes.size() << ") ";
    throw FSError(FSErrorType::FS_EBADVALUESIZE, oss.str());
  }
  auto *idx = bytes.data();
  std::memcpy(this, idx, sizeof(kvfsInodeValue));
}
std::string kvfsInodeValue::pack() const {
  std::string d(sizeof(kvfsInodeValue), L'\0');
  std::memcpy(&d[0], this, d.size());
  return d;
}
}// namespace kvfs