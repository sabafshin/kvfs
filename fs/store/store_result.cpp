/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_result.cpp
 */


#include "store_result.hpp"

namespace {
    void freeString(void* /* buffer */, void* userData) {
        auto str = static_cast<std::string*>(userData);
        delete str;
    }
} // namespace


namespace kvfs{
    [[noreturn]] void StoreResult::throwInvalidError() const {
        // Maybe we should define our own more specific error type in the future
        throw std::domain_error("value not present in store");
    }
}