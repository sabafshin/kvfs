/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.cpp
 */

#include <iostream>
#include "kvfs_rocksdb_store.h"

using std::vector;

namespace kvfs {

kvfsRocksDBStore::kvfsRocksDBStore(const std::string &_db_path) : db_handle(std::make_shared<RocksHandles>(_db_path)) {}

kvfsRocksDBStore::~kvfsRocksDBStore() {
  db_handle->db.reset();
  db_handle.reset();
};

void kvfsRocksDBStore::Close() {
  db_handle->db.reset();
}

bool kvfsRocksDBStore::Put(const std::string &key, const std::string &value) {
  auto status = db_handle->db->Put(rocksdb::WriteOptions(), key, value);
  return status.ok();
}

KVStoreResult kvfsRocksDBStore::Get(const std::string &key) {
  std::string value;
  auto status = db_handle->db->Get(
      rocksdb::ReadOptions(), key, &value);
  if (!status.ok()) {
    if (status.IsNotFound()) {
      // Return an empty StoreResult
      return KVStoreResult();
    }
    throw RocksException(
        status, "failed to get " + key + " from local store");
  }
  return KVStoreResult(std::move(value));
}

bool kvfsRocksDBStore::Delete(const std::string &key) {
  auto status = db_handle->db->Delete(rocksdb::WriteOptions(), key);
  return status.ok();
}

vector<KVStoreResult> kvfsRocksDBStore::GetChildren(const std::string &key) {

  auto val = Get(key);
  if (val.isValid()) {
    kvfsInodeValue dirValue;
    dirValue.parse(val);
    kvfs_file_inode_t prefix = dirValue.fstat_.st_ino;
    vector<KVStoreResult> result;
    auto iter = db_handle->db->NewIterator(rocksdb::ReadOptions());

    for (iter->Seek(reinterpret_cast<const char *>(prefix));
         iter->Valid() && iter->key().starts_with(reinterpret_cast<const char *>(prefix));
         iter->Next()) {
      auto value = iter->value();
      result.emplace_back(KVStoreResult(value.data()));
    }

    delete (iter);
    return result;
  }

  return vector<KVStoreResult>();
}

KVStoreResult kvfsRocksDBStore::GetParent(const std::string &key) {
  auto val = Get(key);

  if (val.isValid()) {
    kvfsInodeValue dv{};
    dv.parse(val);
    auto parent = dv.parent_key_;
    return Get(parent.pack());
  }
  return KVStoreResult();
}

bool kvfsRocksDBStore::Sync() {
  rocksdb::Status status = db_handle->db->SyncWAL();
  return status.ok();
}

bool kvfsRocksDBStore::Compact() {
  auto status = db_handle->db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);

  return status.ok();
}

bool kvfsRocksDBStore::Merge(const std::string &key, const std::string &value) {
  if (this->Get(key).isValid()) {
    auto status = db_handle->db->Delete(rocksdb::WriteOptions(), key);
    status = db_handle->db->Put(rocksdb::WriteOptions(), key, value);
    return status.ok();
  } else {
    return this->Put(key, value);
  }
}

bool kvfsRocksDBStore::DeleteRange(const std::string &start, const std::string &end) {
  auto status = db_handle->db->DeleteRange(rocksdb::WriteOptions(), db_handle->db->DefaultColumnFamily(), start, end);
  return status.ok();
}

namespace {
class RocksDBWriteBatch : public KVStore::WriteBatch {
 public:
  void Put(const std::string &key, const std::string &value) override;

  void Delete(const std::string &key) override;

  void Flush() override;

  explicit RocksDBWriteBatch(std::shared_ptr<RocksHandles> db_handle);

  std::shared_ptr<RocksHandles> db_handle_;
  rocksdb::WriteBatch write_batch;
};

void RocksDBWriteBatch::Flush() {
  auto pending = write_batch.Count();
  if (pending == 0) {
    return;
  }
  auto status = db_handle_->db->Write(rocksdb::WriteOptions(), &write_batch);

  if (!status.ok()) {
    throw RocksException(
        status, "error putting blob in Store");
  }

  write_batch.Clear();
}

RocksDBWriteBatch::RocksDBWriteBatch(const std::shared_ptr<kvfs::RocksHandles> db_handle)
    : KVStore::WriteBatch(), db_handle_(db_handle), write_batch() {}

void RocksDBWriteBatch::Put(const std::string &key, const std::string &value) {
  write_batch.Put(key, value);
}

void RocksDBWriteBatch::Delete(const std::string &key) {
  write_batch.Delete(key);
}

class RocksDBIterator : public KVStore::Iterator {
 public:

  explicit RocksDBIterator(const std::shared_ptr<kvfs::RocksHandles> &db_handle);
  ~RocksDBIterator() override;

  bool Valid() const override;
  void SeekToFirst() override;
  void SeekToLast() override;
  void Seek(const std::string &target) override;
  void SeekForPrev(const std::string &target) override;
  void Next() override;
  void Prev() override;
  std::string key() const override;
  KVStoreResult value() const override;
  bool status() const override;
  bool Refresh() override;

 private:
  std::unique_ptr<rocksdb::Iterator> iterator_;
};

kvfs::RocksDBIterator::RocksDBIterator(const std::shared_ptr<kvfs::RocksHandles> &db_handle)
    : KVStore::Iterator() {
  rocksdb::ReadOptions options;
  options.fill_cache = false;
  iterator_.reset(db_handle->db->NewIterator(options));
}
bool kvfs::RocksDBIterator::Valid() const {
  return iterator_->Valid();
}
void kvfs::RocksDBIterator::SeekToFirst() {
  return iterator_->SeekToFirst();
}
void kvfs::RocksDBIterator::SeekToLast() {
  return iterator_->SeekToLast();
}
void kvfs::RocksDBIterator::Seek(const std::string &target) {
  return iterator_->Seek(target);
}
void kvfs::RocksDBIterator::SeekForPrev(const std::string &target) {
  return iterator_->SeekForPrev(target);
}
void kvfs::RocksDBIterator::Next() {
  return iterator_->Next();
}
void kvfs::RocksDBIterator::Prev() {
  return iterator_->Prev();
}
std::string kvfs::RocksDBIterator::key() const {
  return iterator_->key().ToString();
}
KVStoreResult kvfs::RocksDBIterator::value() const {
  return KVStoreResult(iterator_->value().ToString());
}
bool kvfs::RocksDBIterator::status() const {
  return iterator_->status().ok();
}
bool kvfs::RocksDBIterator::Refresh() {
  return iterator_->Refresh().ok();
}
kvfs::RocksDBIterator::~RocksDBIterator() {
  iterator_.reset();
}

}//namespace

std::unique_ptr<KVStore::WriteBatch> kvfsRocksDBStore::GetWriteBatch() {
  return std::make_unique<RocksDBWriteBatch>(db_handle);
}
std::unique_ptr<KVStore::Iterator> kvfsRocksDBStore::GetIterator() {
  return std::make_unique<RocksDBIterator>(db_handle);
}
bool kvfsRocksDBStore::Destroy() {
  rocksdb::DestroyDB(this->db_handle->db->GetName(), db_handle->db->GetOptions());
  return true;
}

}//namespace kvfs
