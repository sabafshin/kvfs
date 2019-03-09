/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_store.cpp
 */

#include <iostream>
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
//  auto txn = db_handle->db->BeginTransaction(WriteOptions());

//  txn->Put(key, value);

//  auto status = txn->Commit();

  auto status = db_handle->db->Put(rocksdb::WriteOptions(), key, value);
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
//  auto txn = db_handle->db->BeginTransaction(WriteOptions());
//  txn->Delete(key);
//  auto status = txn->Commit();
  auto status = db_handle->db->Delete(rocksdb::WriteOptions(), key);
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

    delete (iter);
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
    return get(parent.pack());
  }
  return StoreResult();
}

bool RocksDBStore::hasKey(const std::string &key) const {
  string value;
  auto status = db_handle->db->KeyMayExist(
      ReadOptions(), key, &value);
  return status;
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
//  auto txn = db_handle->db->BeginTransaction(WriteOptions());
//  txn->Merge(key, value);
  if (this->hasKey(key)) {
//    auto status = db_handle->db->Merge(rocksdb::WriteOptions(), key, value);
//    return status.ok();
//  } else{
    auto status = db_handle->db->Delete(rocksdb::WriteOptions(), key);
    status = db_handle->db->Put(rocksdb::WriteOptions(), key, value);
    return status.ok();
  } else {
    return this->put(key, value);
  }
}

bool RocksDBStore::delete_range(const std::string &start, const std::string &end) {
//  auto txn = db_handle->db->BeginTransaction(WriteOptions());

//  txn->SetSavePoint();
  auto status = db_handle->db->DeleteRange(WriteOptions(), db_handle->db->DefaultColumnFamily(), start, end);
  if (!status.ok()) {
//    txn->RollbackToSavePoint();
    return false;
  }
//  txn->PopSavePoint();

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

class RocksDBIterator : public Store::Iterator {
 public:

  explicit RocksDBIterator(std::shared_ptr<kvfs::RocksHandles> db_handle);
  ~RocksDBIterator() override;

  bool Valid() const override;
  void SeekToFirst() override;
  void SeekToLast() override;
  void Seek(const std::string &target) override;
  void SeekForPrev(const std::string &target) override;
  void Next() override;
  void Prev() override;
  std::string key() const override;
  StoreResult value() const override;
  bool status() const override;
  bool Refresh() override;

  std::shared_ptr<RocksHandles> db_handle_;
 private:
  std::unique_ptr<rocksdb::Iterator> iterator_;
};

kvfs::RocksDBIterator::RocksDBIterator(std::shared_ptr<kvfs::RocksHandles> db_handle)
    : Store::Iterator(),
      db_handle_(db_handle),
      iterator_(std::make_unique<rocksdb::Iterator>(db_handle->db->NewIterator(ReadOptions()))) {}
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
StoreResult kvfs::RocksDBIterator::value() const {
  return StoreResult(iterator_->value().ToString());
}
bool kvfs::RocksDBIterator::status() const {
  return iterator_->status().ok();
}
bool kvfs::RocksDBIterator::Refresh() {
  return iterator_->Refresh().ok();
}
kvfs::RocksDBIterator::~RocksDBIterator() {
  db_handle_.reset();
  iterator_.reset();
}

}//namespace

std::unique_ptr<Store::WriteBatch> RocksDBStore::get_write_batch(size_t buf_size) {
  return std::make_unique<RocksDBWriteBatch>(db_handle, buf_size);
}
std::unique_ptr<Store::Iterator> RocksDBStore::get_iterator() {
  return std::make_unique<RocksDBIterator>(db_handle);
}
bool RocksDBStore::destroy() {
  rocksdb::DestroyDB(this->db_handle->db->GetName(), db_handle->db->GetOptions());
  return true;
}
StoreResult RocksDBStore::get_next_dirent(const std::string &key_, const uint64_t &prefix_, const kvfs_off_t &offset) {
  auto options = ReadOptions();
//  kvfsDirKey upperbound = {prefix_+1, 0};
//  kvfsDirKey lowerbound = {prefix_, 0};
//  rocksdb::Slice* up_ptr = new rocksdb::Slice(upperbound.pack());
//  rocksdb::Slice* lo_ptr = new rocksdb::Slice(lowerbound.pack());
//  options.total_order_seek = true;

//  options.iterate_upper_bound = up_ptr;
//  options.iterate_lower_bound = lo_ptr;

  auto iter = db_handle->db->NewIterator(options);
  iter->Seek(key_);
  for (; iter->Valid(); iter->Next()) {
    auto key_str = iter->key().ToString();
    auto value_str = iter->value().ToString();
    if (iter->key().size() == sizeof(kvfsDirKey) && iter->value().size() == sizeof(kvfsMetaData)) {
      kvfsDirKey key{};
      key.parse(key_str);
      kvfsMetaData md_;
      md_.parse(StoreResult(std::string(value_str)));
//      std::cout << md_.dirent_.d_name << std::endl;
      if (key.inode_ == prefix_ && md_.dirent_.d_off == offset) {
        delete iter;
        return StoreResult(std::string(value_str));
      }
    }
  }
  delete (iter);
//  delete(lo_ptr);
//  delete(up_ptr);
  // not found
  return StoreResult();
}
StoreResult RocksDBStore::seek_at(const std::string &key, const uint64_t &postfix_, const kvfsDirKey &owner) {
  auto iter = db_handle->db->NewIterator(ReadOptions());
  for (iter->Seek(key); iter->Valid(); iter->Next()) {
    auto key_str = iter->key().ToString();
    auto value_str = iter->value().ToString();

    if (iter->value().size() == sizeof(kvfsBlockValue) && iter->key().size() == sizeof(kvfsBlockKey)) {
      kvfsBlockKey key{};
      key.parse(key_str);
      if (key.offset_ != postfix_) {
        continue;
      }
      // it is the block offset we seek so we check owner
      kvfsBlockValue bv_;
      bv_.parse(StoreResult(std::string(value_str)));
      if (bv_.owener_ == owner) {
        // we have found the block
        // return the value
        delete iter;
        return StoreResult(std::string(value_str));
      }
    }
  }

  // not found
  return StoreResult();
}

}//namespace kvfs
