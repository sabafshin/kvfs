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

#include "slice.hpp"
#include "store_entry.hpp"
#include "store_result.hpp"

namespace kvfs {

class Store {
 public:
  virtual bool put(const slice &key, const slice &value) = 0;
  virtual bool merge(const slice &key, const slice &value) = 0;

  virtual StoreResult get(const slice &key) = 0;

  virtual bool delete_(const slice &key) = 0;
  virtual bool delete_range(const slice &start, const slice &end) = 0;

  virtual bool compact() = 0;

  virtual std::vector<StoreResult> get_children(const slice &key) = 0;

  virtual bool get_parent(const slice &key) = 0;

  virtual bool sync() = 0;

  virtual void close() = 0;

  virtual bool hasKey(const slice &key) const = 0;

  class WriteBatch {
   public:
    virtual void put(const slice &key, const slice &value) = 0;

    virtual void delete_(const slice &key) = 0;

    /**
    * Flush any pending data to the store.
    */
    virtual void flush() = 0;

    // Forbidden copy construction/assignment; allow only moves
    WriteBatch(const WriteBatch &) = delete;
    WriteBatch(WriteBatch &&) = default;
    WriteBatch &operator=(const WriteBatch &) = delete;
    WriteBatch &operator=(WriteBatch &&) = default;
    virtual ~WriteBatch() = default;
    WriteBatch() = default;

   private:
    friend class Store;
  };

  virtual std::unique_ptr<WriteBatch> beginWrite(size_t buffer_size = 0) = 0;

};

} // namespace kvfs

#endif //KVFS_STORE_HPP
