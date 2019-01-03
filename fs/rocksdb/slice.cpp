/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   slice.cpp
 */

#include <memory>
#include "slice.hpp"

using rocksdb::Slice;

kvfs::slice::~slice() = default;

kvfs::slice::slice(const std::string &s) {
    value = Slice(s);
}

kvfs::slice::slice(const char *d, size_t n) {
    value = Slice(d, n);
}

kvfs::slice::slice(std::string_view sv) {
    value = Slice(sv);
}

kvfs::slice::slice(const char *s) {
    value = Slice(s);
}






