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

#include <kvfs/kvfs.h>

using namespace rocksdb;
using namespace std;

int main(int argc, char *argv[])
{
    DB *db;
    Options options;
    options.create_if_missing = true;
    Status status = DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());
    options.error_if_exists = true;
    string value = "F";
    Slice key1;
    Slice key2;
    Status s = db->Get(rocksdb::ReadOptions(), key1, &value);
    if (s.ok())
        s = db->Put(rocksdb::WriteOptions(), key2, value);
    if (s.ok())
        s = db->Delete(rocksdb::WriteOptions(), key1);

    cout << "Hello" << errno << endl;
    
    delete db;
    return 0 ;
}