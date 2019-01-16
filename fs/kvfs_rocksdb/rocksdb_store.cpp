/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.cpp
 */

#include "rocksdb_store.hpp"

using std::vector;

namespace kvfs {

RocksDBStore::RocksDBStore(const string &_db_path) : db_handle(_db_path) {}

RocksDBStore::~RocksDBStore() {
  close();
};

void RocksDBStore::close() {
  auto status = db_handle.db->Close();

  if (!status.ok()) {
    db_handle.db->SyncWAL();
  }

  db_handle.db.reset();
}

bool RocksDBStore::put(const slice &key, const slice &value) {

  auto status = db_handle.db->Put(rocksdb::WriteOptions(), key, value);

  return status.ok();
}

StoreResult RocksDBStore::get(const slice &key) {
  string value;
  auto status = db_handle.db->Get(
      ReadOptions(), key, &value);
  if (!status.ok()) {
    if (status.IsNotFound()) {
      // Return an empty StoreResult
      return StoreResult();
    }

    /*throw RocksException::build(
            status, "failed to get " + key.data() + " from local store");*/
  }
  return StoreResult(std::move(value));
}

bool RocksDBStore::delete_(const slice &key) {
  auto status = db_handle.db->Delete(WriteOptions(), key);

  return status.ok();
}

vector<StoreResult> RocksDBStore::get_children(const slice &key) {

  auto val = get(key);
  if (val.isValid()) {
    const auto *dirValue = reinterpret_cast<const dir_value *>(val.asString().data());
    kvfs_file_inode_t prefix = dirValue->this_inode;
    vector<StoreResult> result;
    auto iter = db_handle.db->NewIterator(ReadOptions());

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

bool RocksDBStore::get_parent(const slice &key) {
  /*auto val = get(key.to_slice());

  if (val.isValid()) {
      const auto *dirValue = reinterpret_cast<const dir_value *>(val.asString().data());
      kvfs_file_inode_t inode = dirValue->this_inode - 1;

      if (inode < 0) {
          return false;
      }

      if (inode >= 0) {
          kvfs_file_hash_t hash = dirValue->parent_hash;

          dir_key parent_key;
      }

  }*/
  return false;
}

bool RocksDBStore::hasKey(const slice &key) const {
  string value;
  auto status = db_handle.db->KeyMayExist(
      ReadOptions(), key, &value);
  if (!status) {
    /*throw RocksException::build(
            status, "failed to get ", key, " from local store");*/
    return false;
  }
  return true;
}

bool RocksDBStore::sync() {
  rocksdb::Status status = db_handle.db->SyncWAL();
  return status.ok();
}

bool RocksDBStore::compact() {
  auto status = db_handle.db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);

  return status.ok();
}

bool RocksDBStore::merge(const slice &key, const slice &value) {
  auto status = db_handle.db->Merge(WriteOptions(), key, value);
  return status.ok();
}

bool RocksDBStore::delete_range(const slice &start, const slice &end) {
  auto status = db_handle.db->DeleteRange(WriteOptions(), db_handle.db->DefaultColumnFamily(), start, end);
  return status.ok();
}

namespace {
class RocksDBWriteBatch : public Store::WriteBatch {
 public:
  void put(const slice &key, const slice &value) override;

  void delete_(const slice &key) override;

  void flush() override;

  RocksDBWriteBatch(RocksHandles &db_handle, size_t buffer_size);

  void flush_if_needed();

  RocksHandles &db_handle_;
  rocksdb::WriteBatch write_batch;
  size_t buf_size;
};

void RocksDBWriteBatch::flush() {
  auto pending = write_batch.Count();
  if (pending == 0) {
    return;
  }

  auto status = db_handle_.db->Write(WriteOptions(), &write_batch);

  if (!status.ok()) {
    /*throw RocksException::build(
            status, "error putting blob in Store");*/
  }

  write_batch.Clear();
}

void RocksDBWriteBatch::flush_if_needed() {
  auto needFlush = buf_size > 0 && write_batch.GetDataSize() >= buf_size;

  if (needFlush) {
    flush();
  }
}

RocksDBWriteBatch::RocksDBWriteBatch(kvfs::RocksHandles &db_handle, size_t buffer_size)
    : Store::WriteBatch(), db_handle_(db_handle), write_batch(buffer_size), buf_size(buffer_size) {}

void RocksDBWriteBatch::put(const slice &key, const slice &value) {
  write_batch.Put(key, value);

  flush_if_needed();
}

void RocksDBWriteBatch::delete_(const slice &key) {
  write_batch.Delete(key);
  flush_if_needed();
}

}//namespace

std::unique_ptr<Store::WriteBatch> RocksDBStore::beginWrite(size_t buf_size) {
  return std::make_unique<RocksDBWriteBatch>(db_handle, buf_size);
}

}//namespace kvfs
