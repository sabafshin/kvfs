/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_result.hpp
 */

#ifndef KVFS_STORE_RESULT_HPP
#define KVFS_STORE_RESULT_HPP

#include <string>
#include <stdexcept>

namespace kvfs {
class StoreResult {
 public:
 public:
  /**
   * Construct an invalid StoreResult, representing a key that was not found.
   */
  StoreResult() : valid_(false) {}

  /**
   * Construct a StoreResult from a std::string.
   */
  explicit StoreResult(std::string &&data)
      : valid_(true), data_(std::move(data)) {}

  StoreResult(StoreResult &&) = default;
  StoreResult &operator=(StoreResult &&) = default;

  /**
   * Returns true if the value was found in the store,
   * or false if the key was not present.
   */
  bool isValid() const {
    return valid_;
  }

  /**
   * Get a reference to the std::string result.
   *
   * Throws std::domain_error if the key was not present in the store.
   */
  const std::string &asString() const {
    ensureValid();
    return data_;
  }

  /**
    * Extract the std::string contained in this StoreResult.
    */
  std::string extractValue() {
    ensureValid();
    valid_ = false;
    return std::move(data_);
  }

  /**
    * Throw an exception if this result is not valid
    * (i.e., if the key was not present in the store).
    */
  void ensureValid() const {
    if (!isValid()) {
      throwInvalidError();
    }
  }

  // Forbidden copy constructor and assignment operator
  StoreResult(StoreResult const &) = delete;
  StoreResult &operator=(StoreResult const &) = delete;

 private:
  [[noreturn]] void throwInvalidError() const {
    // Maybe we should define our own more specific error type in the future
    throw std::domain_error("value not present in store");
  }

  // Whether or not the result is value
  // If the key was not found in the store, valid_ will be false.
  bool valid_{false};
  // The std::string containing the data
  std::string data_;
};
}  // namespace kvfs

#endif //KVFS_STORE_RESULT_HPP