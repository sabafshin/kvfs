/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   RocksDBException.cpp
 */

#include <store/RocksDBException.hpp>

namespace kvfs {
    namespace store {

        RocksException::RocksException(
                const rocksdb::Status& status,
                const std::string& msg)
                : status_(status), msg_(msg) {
            fullMsg_ = folly::to<std::string>(msg, "(Status: ", status_.ToString(), ")");
        }

        const char* RocksException::what() const noexcept {
            return fullMsg_.c_str();
        }

        const rocksdb::Status& RocksException::getStatus() const {
            return status_;
        }

        const std::string& RocksException::getMsg() const {
            return msg_;
        }
    }
}