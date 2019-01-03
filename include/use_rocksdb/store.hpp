/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store.hpp
 */

#ifndef KVFS_STORE_HPP
#define KVFS_STORE_HPP

#include <iostream>

#include <rocksdb/db.h>
#include <include/rocksdb/slice_transform.h>


using namespace std;
using namespace rocksdb;

namespace kv {
  class store {

    public:
        store() = default;;

        virtual bool _open(const string &db_name, void** db_handle) = 0 ;

        virtual bool _put(void* db_handle, Slice &key, Slice &value) = 0;

        virtual bool _get(void* db_handle, Slice &key, string* value) = 0;

        virtual bool _delete(void* db_handle ,Slice &key) = 0;

        virtual rocksdb::Iterator *_iterator(void* db_handle) = 0;

        virtual rocksdb::Iterator *prefix_iterator(void* db_handle) = 0;

        ~store() = default;

    };


//    class smart_db_ptr{
//
//        DB* db;
//    public:
//        DB *getDb() const {
//            return db;
//        }
//
//        explicit smart_db_ptr(DB* p = nullptr){
//            db = p;
//        }
//
//        ~smart_db_ptr(){
//            delete db;
//        }
//
//        DB &operator*(){
//            return *db;
//        }
//
//    };
}

#endif //KVFS_STORE_HPP
