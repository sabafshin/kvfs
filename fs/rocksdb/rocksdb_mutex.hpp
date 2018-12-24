/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_mutex.hpp
 */

#ifndef KVFS_ROCKSDB_ROCKSDB_MUTEX_HPP
#define KVFS_ROCKSDB_ROCKSDB_MUTEX_HPP

#include <port/port_posix.h>


namespace kvfs {
    class Mutex {
    public:
        ~Mutex();

        void Lock();
        void Unlock();
        // this will assert if the mutex is not locked
        // it does NOT verify that mutex is held by a calling thread
        void AssertHeld();

    private:
        std::unique_ptr<rocksdb::port::Mutex> mutex;

        // No copying allowed
        Mutex(const Mutex&);
        void operator=(const Mutex&);
    };

    class RWMutex {
    public:
        RWMutex();
        ~RWMutex();

        void ReadLock();
        void WriteLock();
        void ReadUnlock();
        void WriteUnlock();
        void AssertHeld();

    private:
        std::unique_ptr<rocksdb::port::RWMutex> rwMutex; // the underlying platform mutex

        // No copying allowed
        RWMutex(const RWMutex&);
        void operator=(const RWMutex&);
    };
}


#endif //KVFS_ROCKSDB_ROCKSDB_MUTEX_HPP
