// Copyright 2018 Afshin Sabahi. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
/**
 * @brief A secure keyvalue filesystem on top of Rocksdb. The goall of this file
 * system is to be used on SPDK library to achieve exeptional performance. This would be 
 * the fastest filesystem there is with out the IO overhead.
 * 
 * @signiture POSIX_FILE_SYS
 * @file kvfs.cpp
 * @author Afshin Sabahi
 */

#include <util/xxhash.h>
#include <util/crc32c.h>
#include <iostream>
#include <folly/FBString.h>
#include <folly/String.h>

using namespace std;
using namespace rocksdb;
using namespace folly;

struct key {
    int64_t inode;
    uint32_t hash;
};




int main(int argc, char *argv[]) {
    string file1 = "/root/";
    fbstring file2 = "/root/bin";

    struct key key1{
        0,
        XXH32(file1.data(), static_cast<int>(file1.size()), 0)
    };

    struct key key2{
        0,
        XXH32(file2.data(), static_cast<int>(file2.size()), 0)
    };

    cout << "First file:  " << file1 << " XXHash32    is :[" << to_string(key1.hash) << "]" << endl;
    cout << crc32c::IsFastCrc32Supported() << endl;
    key1.hash = crc32c::Value(file1.data(), file1.size());
    cout << "First file:  " << file1 << " CRC32c      is :[" << key1.hash << "]" << endl;
    cout << "Second file: " << file2 << " XXHash32    is :[" << key2.hash << "]" << endl;

    return 0;
}