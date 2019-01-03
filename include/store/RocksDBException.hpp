/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   RocksDBException.hpp
 */
#pragma once

#include <folly/String.h>
#include <rocksdb/db.h>

using std::string;

namespace kvfs{
    namespace store{
        class RocksException : public std::exception {
        public:
            RocksException(const rocksdb::Status& status, const std::string& msg);

            template <typename... Args>
            static RocksException build(const rocksdb::Status& status, Args&&... args) {
                return RocksException(
                        status, folly::to<std::string>(std::forward<Args>(args)...));
            }

            template <typename... Args>
            static void check(const rocksdb::Status& status, Args&&... args) {
                if (UNLIKELY(!status.ok())) {
                    throw build(status, std::forward<Args>(args)...);
                }
            }

            const char* what() const noexcept override;
            const rocksdb::Status& getStatus() const;
            const std::string& getMsg() const;

        private:
            rocksdb::Status status_;
            std::string msg_;
            std::string fullMsg_;
        };
    }
}