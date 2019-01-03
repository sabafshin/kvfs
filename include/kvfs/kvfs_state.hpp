/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   nvfs_state.hpp
 */

#ifndef KVFS_STATE_HPP
#define KVFS_STATE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <folly/String.h>
#include <folly/AtomicUnorderedMap.h>
#include <folly/Format.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include <errno.h>
#include <store/RocksDBStore.hpp>
#include <utils/properties.hpp>
#include <kvfs/inode.hpp>

using folly::StringPiece;
using folly::ByteRange;
using folly::IOBuf;
using folly::Optional;
using folly::StringPiece;
using folly::io::Cursor;

using namespace kvfs::store;

namespace kvfs{
    struct FileSystemState {
        StringPiece metadir_;
        StringPiece datadir_;
        StringPiece mountdir_;

        std::unique_ptr<RocksDbStore> _store;

        kvfs_inode_t max_inode_num;
        int threshold_;

        FileSystemState();

        ~FileSystemState();

        virtual int Setup(Properties& prop) = 0;

        virtual void Destroy() = 0;

        RocksDbStore* GetMetaDB() {
            return _store.get();
        }


        const StringPiece& GetDataDir() {
            return datadir_;
        }

        const int GetThreshold() {
            return threshold_;
        }

        bool IsEmpty() {
            return (max_inode_num == 0);
        }

        virtual kvfs_inode_t NewInode() = 0;
    };

}



#endif //KVFS_STATE_HPP
