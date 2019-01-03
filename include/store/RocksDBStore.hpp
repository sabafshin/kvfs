/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   RocksDBStore.hpp
 */

#pragma once

#include <store/RocksDBHandles.hpp>
#include <store/StoreResult.hpp>
#include <folly/FBVector.h>
#include <folly/FBString.h>
#include <folly/Conv.h>
#include <utils/properties.hpp>
#include <folly/logging/xlog.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <store/RocksDBException.hpp>
#include <time.h>

using rocksdb::Slice;
using folly::ByteRange;

namespace kvfs {
    namespace store {

        struct RocksDBIterator {
            std::unique_ptr<rocksdb::Iterator> iter_;

            explicit RocksDBIterator(rocksdb::Iterator *it) {
//                XLOG(INFO) << "Acquiring Iterator" << ".\n";
                iter_.reset(it);
//                XLOG(INFO) << "Iterator pointer acquired.\n";
            }

            ~RocksDBIterator() {
                iter_.reset();
//                XLOG(INFO) << "Iterator pointer released.\n";
            }

            RocksDBIterator(const RocksDBIterator &) = delete;

            RocksDBIterator &operator=(const RocksDBIterator &) = delete;

            RocksDBIterator(RocksDBIterator &&) = default;

            RocksDBIterator &operator=(RocksDBIterator &&) = default;
        };

        class RocksDbStore {
        public:
            RocksDbStore();

            virtual ~RocksDbStore();

            void SetProperties(const Properties &p) {
                _p = p;
            }

            virtual int Init() = 0;

            virtual void Cleanup() = 0;

            virtual void close() = 0;

            virtual StoreResult get(ByteRange key) = 0;

            virtual bool put(
                    ByteRange key, ByteRange value) = 0;

            virtual int Sync() = 0;

            virtual bool beginWrite(rocksdb::WriteBatch write_batch) = 0;

            virtual bool Delete(ByteRange key) = 0;

            virtual RocksDBIterator *GetNewIterator() = 0;

            virtual bool hasKey(ByteRange key) const = 0;

            virtual void Compact() = 0;

            virtual void Report() = 0;

            virtual bool GetStat(const std::string &stat, std::string *value) {
                return dbHandles_.db->GetProperty(stat, value);
            }

            virtual bool GetMetrics(std::string *value) = 0;

        private:
            RocksHandles dbHandles_;
            Properties   _p;
            bool         writeahead{};
            time_t       last_sync_time{};
            time_t       sync_time_limit{};
            int          async_data_size{};
            int          sync_size_limit{};
        };

    } //namespace store
} // namespace kvfs