/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_leveldb_store.cpp
 */

#include <leveldb/write_batch.h>
#include "kvfs_leveldb_store.h"

kvfs::kvfsLevelDBStore::kvfsLevelDBStore(const std::string &db_path)
    : db_handle(std::make_unique<LevelDBHandles>(db_path)), db_name(db_path) {
}
kvfs::kvfsLevelDBStore::~kvfsLevelDBStore() {
  db_handle.reset();
}
bool kvfs::kvfsLevelDBStore::put(const std::string &key, const std::string &value) {
  leveldb::Status status = db_handle->db->Put(leveldb::WriteOptions(), key, value);
  return status.ok();
}
void kvfs::kvfsLevelDBStore::close() {
  db_handle.reset();
}
bool kvfs::kvfsLevelDBStore::merge(const std::string &key, const std::string &value) {
  db_handle->db->Delete(leveldb::WriteOptions(), key);
  leveldb::Status status = db_handle->db->Put(leveldb::WriteOptions(), key, value);
  return status.ok();
}
kvfs::StoreResult kvfs::kvfsLevelDBStore::get(const std::string &key) {
  std::string value;
  leveldb::Status status = db_handle->db->Get(leveldb::ReadOptions(), key, &value);
  if (!status.ok()) {
    if (status.IsNotFound()) {
      // Return an empty StoreResult
      return StoreResult();
    }
    throw LevelDBException(
        status, "failed to get " + key + " from local store");
  }
  return StoreResult(std::move(value));
}
bool kvfs::kvfsLevelDBStore::delete_(const std::string &key) {
  auto status = db_handle->db->Delete(leveldb::WriteOptions(), key);
  return status.ok();
}
bool kvfs::kvfsLevelDBStore::delete_range(const std::string &start, const std::string &end) {
  return false;
}
std::vector<kvfs::StoreResult> kvfs::kvfsLevelDBStore::get_children(const std::string &key) {
  return std::vector<kvfs::StoreResult>();
}
kvfs::StoreResult kvfs::kvfsLevelDBStore::get_parent(const std::string &key) {
  return kvfs::StoreResult();
}
bool kvfs::kvfsLevelDBStore::sync() {
  leveldb::WriteOptions options;
  options.sync = true;
  leveldb::Status status = db_handle->db->Put(options, "sync", "");
  return status.ok();
}
bool kvfs::kvfsLevelDBStore::compact() {
  db_handle->db->CompactRange(nullptr, nullptr);
  return true;
}
bool kvfs::kvfsLevelDBStore::destroy() {
  leveldb::DestroyDB(db_name, leveldb::Options());
  return true;
}
namespace {
class LevelDBWriteBatch : public kvfs::Store::WriteBatch {
 public:
  void put(const std::string &key, const std::string &value) override;

  void delete_(const std::string &key) override;

  void flush() override;

  LevelDBWriteBatch(std::shared_ptr<kvfs::LevelDBHandles> db_handle);

  std::shared_ptr<kvfs::LevelDBHandles> db_handle_;
  leveldb::WriteBatch write_batch;
};

void LevelDBWriteBatch::flush() {
  auto pending = write_batch.ApproximateSize();
  if (pending == 0) {
    return;
  }

  auto status = db_handle_->db->Write(leveldb::WriteOptions(), &write_batch);

  if (!status.ok()) {
    throw kvfs::LevelDBException(
        status, "error putting blob in Store");
  }

  write_batch.Clear();
}

LevelDBWriteBatch::LevelDBWriteBatch(const std::shared_ptr<kvfs::LevelDBHandles> db_handle)
    : kvfs::Store::WriteBatch(), db_handle_(db_handle), write_batch() {}

void LevelDBWriteBatch::put(const std::string &key, const std::string &value) {
  write_batch.Put(key, value);

}

void LevelDBWriteBatch::delete_(const std::string &key) {
  write_batch.Delete(key);
}

class LevelDBIterator : public kvfs::Store::Iterator {
 public:

  explicit LevelDBIterator(const std::shared_ptr<kvfs::LevelDBHandles> &db_handle);
  ~LevelDBIterator() override;

  bool Valid() const override;
  void SeekToFirst() override;
  void SeekToLast() override;
  void Seek(const std::string &target) override;
  void Next() override;
  void Prev() override;
  std::string key() const override;
  kvfs::StoreResult value() const override;
  bool status() const override;
  void SeekForPrev(const std::string &target) override;
  bool Refresh() override;

 private:
  std::unique_ptr<leveldb::Iterator> iterator_;
};

LevelDBIterator::LevelDBIterator(const std::shared_ptr<kvfs::LevelDBHandles> &db_handle)
    : kvfs::Store::Iterator(),
      iterator_(db_handle->db->NewIterator(leveldb::ReadOptions())) {}
bool LevelDBIterator::Valid() const {
  return iterator_->Valid();
}
void LevelDBIterator::SeekToFirst() {
  return iterator_->SeekToFirst();
}
void LevelDBIterator::SeekToLast() {
  return iterator_->SeekToLast();
}
void LevelDBIterator::Seek(const std::string &target) {
  return iterator_->Seek(target);
}
void LevelDBIterator::Next() {
  return iterator_->Next();
}
void LevelDBIterator::Prev() {
  return iterator_->Prev();
}
std::string LevelDBIterator::key() const {
  return iterator_->key().ToString();
}
kvfs::StoreResult LevelDBIterator::value() const {
  return kvfs::StoreResult(iterator_->value().ToString());
}
bool LevelDBIterator::status() const {
  return iterator_->status().ok();
}
LevelDBIterator::~LevelDBIterator() {
  iterator_.reset();
}
void LevelDBIterator::SeekForPrev(const std::string &target) {
}
bool LevelDBIterator::Refresh() {
  return false;
}

}//namespace

std::unique_ptr<kvfs::Store::WriteBatch> kvfs::kvfsLevelDBStore::get_write_batch() {
  return std::make_unique<LevelDBWriteBatch>(db_handle);
}
std::unique_ptr<kvfs::Store::Iterator> kvfs::kvfsLevelDBStore::get_iterator() {
  return std::make_unique<LevelDBIterator>(db_handle);
}