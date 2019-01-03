/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   slice.hpp
 */

#ifndef KVFS_SLICE_HPP
#define KVFS_SLICE_HPP

#include <string>
#include <cstring>
#include <assert.h>
#include <rocksdb/slice.h>

namespace kvfs {
    class Slice : public rocksdb::Slice {
    public:
        // Create an empty slice.
        Slice() : data_(""), size_(0) {}

        // Create a slice that refers to d[0,n-1].
        Slice(const char *d, size_t n) : data_(d), size_(n) {}

        // Create a slice that refers to the contents of "s"
        /* implicit */
        explicit Slice(const std::string &s) : data_(s.data()), size_(s.size()) {}

        // Create a slice that refers to s[0,strlen(s)-1]
        /* implicit */
        explicit Slice(const char *s) : data_(s) {
            size_ = (s == nullptr) ? 0 : strlen(s);
        }

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

        // Change this slice to refer to an empty array
        void clear() {
            data_ = "";
            size_ = 0;
        }

        // Drop the first "n" bytes from this slice.
        void remove_prefix(size_t n) {
            assert(n <= size());
            data_ += n;
            size_ -= n;
        }

        void remove_suffix(size_t n) {
            assert(n <= size());
            size_ -= n;
        }

        // Three-way comparison.  Returns value:
        //   <  0 iff "*this" <  "b",
        //   == 0 iff "*this" == "b",
        //   >  0 iff "*this" >  "b"
        int compare(const Slice &b) const;

        // Return true iff "x" is a prefix of "*this"
        bool starts_with(const Slice &x) const {
            return ((size_ >= x.size_) &&
                    (memcmp(data_, x.data_, x.size_) == 0));
        }

        bool ends_with(const Slice &x) const {
            return ((size_ >= x.size_) &&
                    (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
        }

        // Compare two slices and returns the first byte where they differ
        size_t difference_offset(const Slice &b) const;

    private:
        const char *data_;
        size_t     size_;

        // Intentionally copyable
    };

    inline bool operator==(const Slice &x, const Slice &y) {
        return ((x.size() == y.size()) &&
                (memcmp(x.data(), y.data(), x.size()) == 0));
    }

    inline bool operator!=(const Slice &x, const Slice &y) {
        return !(x == y);
    }

    inline int Slice::compare(const Slice &b) const {
        assert(data_ != nullptr && b.data_ != nullptr);
        const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
        int          r       = memcmp(data_, b.data_, min_len);
        if (r == 0) {
            if (size_ < b.size_) { r = -1; }
            else if (size_ > b.size_) r = +1;
        }
        return r;
    }

    inline size_t Slice::difference_offset(const Slice &b) const {
        size_t       off = 0;
        const size_t len = (size_ < b.size_) ? size_ : b.size_;
        for (; off < len; off++) {
            if (data_[off] != b.data_[off]) break;
        }
        return off;
    }
}

#endif //KVFS_SLICE_HPP
