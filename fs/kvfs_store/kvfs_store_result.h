/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_result.hpp
 */

#ifndef KVFS_STORE_RESULT_H
#define KVFS_STORE_RESULT_H

#include <string>
#include <stdexcept>
#include <kvfs/fs_error.h>

namespace kvfs {
class KVStoreResult {
 public:
  /**
   * Construct an invalid StoreResult, representing a key that was not found.
   */
  KVStoreResult() : valid_(false) {}

  /**
   * Construct a StoreResult from a std::string.
   */
  explicit KVStoreResult(std::string &&data);

  KVStoreResult(KVStoreResult &&) = default;
  KVStoreResult &operator=(KVStoreResult &&) = default;

  /**
   * Returns true if the value was found in the store,
   * or false if the key was not present.
   */
  bool isValid() const;

  /**
   * Get a reference to the std::string result.
   *
   * Throws FSError if the key was not present in the store.
   */
  const std::string &asString() const;

  /**
    * Throw an exception if this result is not valid
    * (i.e., if the key was not present in the store).
    */
  void ensureValid() const;

  // Forbidden copy constructor and assignment operator
  KVStoreResult(KVStoreResult const &) = delete;
  KVStoreResult &operator=(KVStoreResult const &) = delete;

 private:
  // Whether or not the result is value
  // If the key was not found in the store, valid_ will be false.
  bool valid_{false};
  // The std::string containing the data
  std::string data_;
};
}  // namespace kvfs

#endif //KVFS_STORE_RESULT_H