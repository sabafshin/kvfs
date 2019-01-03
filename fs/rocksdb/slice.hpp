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
    struct slice {
        rocksdb::Slice value;

        ~slice();

        explicit slice(const std::string& s);
        explicit slice(const char* d, size_t n);
        explicit slice(std::string_view sv);
        explicit slice(const char* s);

        slice(const slice &) = default;

        slice &operator=(const slice &) = delete;

        slice(slice &&) = default;

        slice &operator=(slice &&) = default;
    };
}

#endif //KVFS_SLICE_HPP
