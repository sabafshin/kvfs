/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_inode.hpp
 */

#ifndef KVFS_INODE_HPP
#define KVFS_INODE_HPP

#include <sys/stat.h>
#include <limits.h>
#include <stdint.h>
#include <folly/String.h>
#include <folly/Range.h>
#include <stdint.h>
#include <array>
#include <rocksdb/slice.h>
#include <util/xxhash.h>

namespace kvfs {

    typedef uint_fast64_t kvfs_inode_t;
    typedef uint_fast64_t kvfs_hash_t;
    typedef struct stat kvfs_stat_t;

    static const char PATH_DELIMITER = '/';
    static const int INODE_PADDING = 104;
    static const int MAX_PATH_LEN = 255;
    static const kvfs_inode_t ROOT_INODE_ID = 0;
    static const int NUM_FILES_IN_DATADIR_BITS = 14;
    static const int NUM_FILES_IN_DATADIR = 16384;
    static const int MAX_OPEN_FILES = 512;
    static const char* ROOT_INODE_STAT = "/tmp/";


    struct _meta_key_t {
        kvfs_inode_t inode_id;
        kvfs_hash_t hash_id;

        const folly::StringPiece ToString() const {
            return folly::StringPiece((const char *) this, sizeof(_meta_key_t));
        }

        folly::ByteRange ToByteRange() const {
            return (folly::ByteRange) folly::Range((const char *) this, sizeof(_meta_key_t));
        }

        rocksdb::Slice ToSlice() const {
            return rocksdb::Slice((const char *) this, sizeof(_meta_key_t));
        }
    };

    struct kvfs_inode_header {
        kvfs_stat_t fstat;
        char padding[INODE_PADDING];
        bool has_blob;
        uint_fast64_t namelen;
        folly::fbstring blocks;
    };

    static const size_t KVFS_INODE_HEADER_SIZE = sizeof(kvfs_inode_header);
    static const size_t KVFS_INODE_ATTR_SIZE = sizeof(struct stat);

    struct _inode_val_t {
        size_t size;
        char* value;

        _inode_val_t() : value(nullptr), size(0) {}

        folly::ByteRange ToByteRange() const {
            return (folly::ByteRange) folly::Range((const char *) value, size);
        }

        folly::StringPiece ToString() const {
            return folly::StringPiece(value, size);
        }
    };

}



#endif //KVFS_INODE_HPP
