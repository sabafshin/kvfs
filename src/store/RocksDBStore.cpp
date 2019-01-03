/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   RocksDBStore.cpp
 */

#include <store/RocksDBStore.hpp>

using folly::ByteRange;
using folly::IOBuf;
using folly::StringPiece;
using rocksdb::ReadOptions;
using rocksdb::Slice;
using rocksdb::SliceParts;
using rocksdb::WriteBatch;
using rocksdb::WriteOptions;
using std::string;
using std::unique_ptr;


namespace kvfs {
    namespace store {
        rocksdb::Slice _createSlice(const ByteRange &bytes) {
            return Slice(reinterpret_cast<const char *>(bytes.data()), bytes.size());
        }
        RocksDbStore::RocksDbStore() : dbHandles_("tmp/kvfsdb") {}

        RocksDbStore::~RocksDbStore() = default;

        void RocksDbStore::close() {
            dbHandles_.db.reset();
        }

        int RocksDbStore::Init() {
            assert(dbHandles_.db != nullptr);

            writeahead      = true;
            sync_size_limit = -1;
            sync_time_limit = 5;
            last_sync_time  = time(nullptr);
            async_data_size = 0;

            return 0;
        }

        void RocksDbStore::Cleanup() {
            RocksDbStore::close();
        }

        StoreResult RocksDbStore::get(ByteRange key) {

            string value;
            auto   status = dbHandles_.db->Get(
                    ReadOptions(), _createSlice(key), &value);
            if (!status.ok()) {
                if (status.IsNotFound()) {
                    // Return an empty StoreResult
                    return StoreResult();
                }

                throw RocksException::build(
                        status, "failed to get ", folly::hexlify(key), " from local store");
            }
            return StoreResult(std::move(value));
        }

        RocksDBIterator *RocksDbStore::GetNewIterator() {
//            XLOG(INFO) << "Initiated Iterator";
            rocksdb::Iterator *it = dbHandles_.db->NewIterator(ReadOptions());
            return new RocksDBIterator(it);
        }

        bool RocksDbStore::hasKey(ByteRange key) const {
            string value;
            auto   status = dbHandles_.db->Get(
                    ReadOptions(), _createSlice(key), &value);
            if (!status.ok()) {
                if (status.IsNotFound()) {
                    return false;
                }

                throw RocksException::build(
                        status, "failed to get ", folly::hexlify(key), " from local store");
            }
            return true;
        }

        int RocksDbStore::Sync() {
            WriteOptions writeOptions;
            writeOptions.sync = true;
            rocksdb::Status status = dbHandles_.db->Put(writeOptions, "sync", "");
            return status.ok();
        }

        bool RocksDbStore::put(
                ByteRange key, ByteRange value) {

//            XLOG(INFO) << "Beginning Put Operation";

            WriteOptions write_options;
            if (sync_size_limit > 0) {
                async_data_size += key.size() + value.size();
                if (async_data_size > sync_size_limit) {
                    write_options.sync = true;
                    async_data_size = 0;
                }
            }
            else if (sync_time_limit > 0) {
                time_t now = time(nullptr);
                if (now - last_sync_time > sync_time_limit) {
                    write_options.sync = true;
                    last_sync_time = now;
                }
            }
            auto         status =
                                 dbHandles_.
                                                   db->Put(write_options, _createSlice(key), _createSlice(value));

            if (!status.ok()) {
//                XLOG(CRITICAL) << "Failed to Complete Put Operation";
                return false;
            }

            return true;
        }

        bool RocksDbStore::Delete(ByteRange key) {
            const int *data     = (const int *) key.data();

//            XLOG(INFO) << "Delete %d %x\n" << data[0] << data[1];

            WriteOptions write_options;
            auto         status = dbHandles_.db->Delete(write_options, _createSlice(key));

            if (!status.ok()) {
//                XLOG(CRITICAL) << "Failed to do delete operation on key: (" << key.toString() << ")";
                return false;
            }

            return true;
        }

        bool RocksDbStore::beginWrite(WriteBatch batch) {
            WriteOptions write_options;
            auto         s = dbHandles_.db->Write(write_options, &batch);
            if (!s.ok()) {
//                XLOG(CRITICAL) << "Failed to complete WriteBatch operation.";
                return false;
            }
            return true;
        }

        void RocksDbStore::Report() {
            std::string result;
            dbHandles_.db->GetProperty(rocksdb::Slice("rocksdb.stats"), &result);
//            XLOG(INFO) << "\n%s\n" << result.c_str();
        }

        void RocksDbStore::Compact() {
            dbHandles_.db->CompactRange(rocksdb::CompactRangeOptions(), nullptr, nullptr);
        }

        bool RocksDbStore::GetMetrics(std::string *value) {
            return dbHandles_.db->GetProperty(Slice("rocksdb.stats"), value);
        }

    } // namespace store
} // namespace kvfs