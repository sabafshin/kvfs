/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store.hpp
 */

#ifndef KVFS_STORE_HPP
#define KVFS_STORE_HPP

#include <memory>
#include <vector>
#include <string>

#include "kvfs_store_entry.h"
#include "kvfs_store_result.h"

namespace kvfs {

class KVStore {
 public:
  virtual bool Put(const std::string &key, const std::string &value) = 0;
  virtual bool Merge(const std::string &key, const std::string &value) = 0;

  virtual KVStoreResult Get(const std::string &key) = 0;

  virtual bool Delete(const std::string &key) = 0;
  virtual bool DeleteRange(const std::string &start, const std::string &end) = 0;

  virtual bool Compact() = 0;

  virtual std::vector<KVStoreResult> GetChildren(const std::string &key) = 0;

  virtual KVStoreResult GetParent(const std::string &key) = 0;

  virtual bool Sync() = 0;

  virtual void Close() = 0;

  virtual bool Destroy() = 0;
  class WriteBatch {
   public:
    virtual void Put(const std::string &key, const std::string &value) = 0;
    virtual void Delete(const std::string &key) = 0;

    /**
    * Flush any pending data to the store.
    */
    virtual void Flush() = 0;

    // Forbidden copy construction/assignment; allow only moves
    WriteBatch(const WriteBatch &) = delete;
    WriteBatch(WriteBatch &&) = default;
    WriteBatch &operator=(const WriteBatch &) = delete;
    WriteBatch &operator=(WriteBatch &&) = default;
    virtual ~WriteBatch() = default;
    WriteBatch() = default;

   private:
    friend class KVStore;
  };

  virtual std::unique_ptr<WriteBatch> GetWriteBatch() = 0;

  class Iterator {
   public:

    virtual bool Valid() const = 0;

    virtual void SeekToFirst() = 0;

    virtual void SeekToLast() = 0;

    virtual void Seek(const std::string &target) = 0;

    virtual void SeekForPrev(const std::string &target) = 0;

    virtual void Next() = 0;

    virtual void Prev() = 0;

    virtual std::string key() const = 0;

    virtual KVStoreResult value() const = 0;

    virtual bool status() const = 0;

    virtual bool Refresh() = 0;

    // No copying allowed
    // Forbidden copy construction/assignment; allow only moves
    Iterator(const Iterator &) = delete;
    Iterator(Iterator &&) = default;
    Iterator &operator=(const Iterator &) = delete;
    Iterator &operator=(Iterator &&) = default;
    virtual ~Iterator() = default;
    Iterator() = default;

   private:
    friend class KVStore;
  };

  virtual std::unique_ptr<Iterator> GetIterator() = 0;

};

} // namespace kvfs

#endif //KVFS_STORE_HPP
