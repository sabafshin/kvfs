/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.cpp
 */

#include "rocksdb_store.h"

using std::vector;

namespace kvfs {

RocksDBStore::RocksDBStore(const string &_db_path) : db_handle(std::make_shared<RocksHandles>(_db_path)) {}

RocksDBStore::~RocksDBStore() {
  db_handle->db.reset();
  db_handle.reset();
};

void RocksDBStore::close() {
  db_handle->db.reset();
//  db_handle.reset();
}

bool RocksDBStore::put(const std::string &key, const std::string &value) {
  auto txn = db_handle->db->BeginTransaction(WriteOptions());

  txn->Put(key, value);

  auto status = txn->Commit();

  return status.ok();
}

StoreResult RocksDBStore::get(const std::string &key) {
  string value;
  auto status = db_handle->db->Get(
      ReadOptions(), key, &value);
  if (!status.ok()) {
    if (status.IsNotFound()) {
      // Return an empty StoreResult
      return StoreResult();
    }

    throw RocksException(
        status, "failed to get " + key + " from local store");
  }
  return StoreResult(std::move(value));
}

bool RocksDBStore::delete_(const std::string &key) {
  auto txn = db_handle->db->BeginTransaction(WriteOptions());
  txn->Delete(key);
  auto status = txn->Commit();
  return status.ok();
}

vector<StoreResult> RocksDBStore::get_children(const std::string &key) {

  auto val = get(key);
  if (val.isValid()) {
    kvfsMetaData dirValue;
    dirValue.parse(val);
    kvfs_file_inode_t prefix = dirValue.fstat_.st_ino;
    vector<StoreResult> result;
    auto iter = db_handle->db->NewIterator(ReadOptions());

    for (iter->Seek(reinterpret_cast<const char *>(prefix));
         iter->Valid() && iter->key().starts_with(reinterpret_cast<const char *>(prefix));
         iter->Next()) {
      auto value = iter->value();
      result.emplace_back(StoreResult(value.data()));
    }

    return result;
  }

  return vector<StoreResult>();
}

StoreResult RocksDBStore::get_parent(const std::string &key) {
  auto val = get(key);

  if (val.isValid()) {
    kvfsMetaData dv{};
    dv.parse(val);
    auto parent = dv.parent_key_;
    return get(parent.to_string());
  }
  return StoreResult();
}

bool RocksDBStore::hasKey(const std::string &key) const {
  string value;
  auto status = db_handle->db->KeyMayExist(
      ReadOptions(), key, &value);
  if (!status) {
    std::string msg = "failed to get " + key + " from local store";
    throw RocksException(
        status, msg);
  }
  return true;
}

bool RocksDBStore::sync() {
  rocksdb::Status status = db_handle->db->SyncWAL();
  return status.ok();
}

bool RocksDBStore::compact() {
  auto status = db_handle->db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);

  return status.ok();
}

bool RocksDBStore::merge(const std::string &key, const std::string &value) {
  auto txn = db_handle->db->BeginTransaction(WriteOptions());
  txn->Merge(key, value);
  auto status = txn->Commit();
  return status.ok();
}

bool RocksDBStore::delete_range(const std::string &start, const std::string &end) {
  auto txn = db_handle->db->BeginTransaction(WriteOptions());

  txn->SetSavePoint();
  auto status = db_handle->db->DeleteRange(WriteOptions(), db_handle->db->DefaultColumnFamily(), start, end);
  if (!status.ok()) {
    txn->RollbackToSavePoint();
    return false;
  }
  txn->PopSavePoint();

  return true;
}

namespace {
class RocksDBWriteBatch : public Store::WriteBatch {
 public:
  void put(const std::string &key, const std::string &value) override;

  void delete_(const std::string &key) override;

  void flush() override;

  RocksDBWriteBatch(std::shared_ptr<RocksHandles> db_handle, size_t buffer_size);

  void flush_if_needed();

  std::shared_ptr<RocksHandles> db_handle_;
  rocksdb::WriteBatch write_batch;
  size_t buf_size;
};

void RocksDBWriteBatch::flush() {
  auto pending = write_batch.Count();
  if (pending == 0) {
    return;
  }

  auto status = db_handle_->db->Write(WriteOptions(), &write_batch);

  if (!status.ok()) {
    throw RocksException(
        status, "error putting blob in Store");
  }

  write_batch.Clear();
}

void RocksDBWriteBatch::flush_if_needed() {
  auto needFlush = buf_size > 0 && write_batch.GetDataSize() >= buf_size;

  if (needFlush) {
    flush();
  }
}

RocksDBWriteBatch::RocksDBWriteBatch(const std::shared_ptr<kvfs::RocksHandles> db_handle, size_t buffer_size)
    : Store::WriteBatch(), db_handle_(db_handle), write_batch(buffer_size), buf_size(buffer_size) {}

void RocksDBWriteBatch::put(const std::string &key, const std::string &value) {
  write_batch.Put(key, value);

  flush_if_needed();
}

void RocksDBWriteBatch::delete_(const std::string &key) {
  write_batch.Delete(key);
  flush_if_needed();
}

}//namespace

std::unique_ptr<Store::WriteBatch> RocksDBStore::beginWrite(size_t buf_size) {
  return std::make_unique<RocksDBWriteBatch>(db_handle, buf_size);
}

}//namespace kvfs
