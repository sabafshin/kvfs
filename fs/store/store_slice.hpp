/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_slice.hpp
 */

#ifndef KVFS_STORE_SLICE_HPP
#define KVFS_STORE_SLICE_HPP

#include <assert.h>
#include <cstdio>
#include <stddef.h>
#include <string.h>
#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

namespace kvfs {
class rocksdb_slice {
 public:
  // Create an empty rocksdb_slice.
  rocksdb_slice() : data_(""), size_(0) {}

  // Create a rocksdb_slice that refers to d[0,n-1].
  rocksdb_slice(const char *d, size_t n) : data_(d), size_(n) {}

  // Create a rocksdb_slice that refers to the contents of "s"
  /* implicit */
  rocksdb_slice(const std::string &s) : data_(s.data()), size_(s.size()) {}

#ifdef __cpp_lib_string_view
  // Create a rocksdb_slice that refers to the same contents as "sv"
  /* implicit */
  rocksdb_slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {}
#endif

  // Create a rocksdb_slice that refers to s[0,strlen(s)-1]
  /* implicit */
  rocksdb_slice(const char *s) : data_(s) {
    size_ = (s == nullptr) ? 0 : strlen(s);
  }

  // Create a single rocksdb_slice from SliceParts using buf as storage.
  // buf must exist as long as the returned Slice exists.
  rocksdb_slice(const struct SliceParts &parts, std::string *buf);

  // Return a pointer to the beginning of the referenced data
  const char *data() const { return data_; }

  // Return the length (in bytes) of the referenced data
  size_t size() const { return size_; }

  // Return true iff the length of the referenced data is zero
  bool empty() const { return size_ == 0; }

  // Return the ith byte in the referenced data.
  // REQUIRES: n < size()
  char operator[](size_t n) const {
    assert(n < size());
    return data_[n];
  }

  // Change this rocksdb_slice to refer to an empty array
  void clear() {
    data_ = "";
    size_ = 0;
  }

  // Drop the first "n" bytes from this rocksdb_slice.
  void remove_prefix(size_t n) {
    assert(n <= size());
    data_ += n;
    size_ -= n;
  }

  void remove_suffix(size_t n) {
    assert(n <= size());
    size_ -= n;
  }

  // Return a string that contains the copy of the referenced data.
  // when hex is true, returns a string of twice the length hex encoded (0-9A-F)
  std::string ToString(bool hex = false) const;

#ifdef __cpp_lib_string_view
  // Return a string_view that references the same data as this rocksdb_slice.
  std::string_view ToStringView() const {
    return std::string_view(data_, size_);
  }
#endif

  // Decodes the current rocksdb_slice interpreted as an hexadecimal string into result,
  // if successful returns true, if this isn't a valid hex string
  // (e.g not coming from Slice::ToString(true)) DecodeHex returns false.
  // This rocksdb_slice is expected to have an even number of 0-9A-F characters
  // also accepts lowercase (a-f)
  bool DecodeHex(std::string *result) const;

  // Three-way comparison.  Returns value:
  //   <  0 iff "*this" <  "b",
  //   == 0 iff "*this" == "b",
  //   >  0 iff "*this" >  "b"
  int compare(const rocksdb_slice &b) const;

  // Return true iff "x" is a prefix of "*this"
  bool starts_with(const rocksdb_slice &x) const {
    return ((size_ >= x.size_) &&
        (memcmp(data_, x.data_, x.size_) == 0));
  }

  bool ends_with(const rocksdb_slice &x) const {
    return ((size_ >= x.size_) &&
        (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
  }

  // Compare two slices and returns the first byte where they differ
  size_t difference_offset(const rocksdb_slice &b) const;

  // private: make these public for rocksdbjni access
  const char *data_;
  size_t size_;

  // Intentionally copyable
};
}

#endif //KVFS_STORE_SLICE_HPP
