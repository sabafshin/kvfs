/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   inode_mutex.hpp
 */

#ifndef KVFS_INODE_MUTEX_HPP
#define KVFS_INODE_MUTEX_HPP

#include <stdint.h>
#include <pthread.h>
#include <kvfs/inode.hpp>

namespace kvfs {

    class RWLock{
    public:
        RWLock();
        ~RWLock();

        void ReadLock();
        void WriteLock();
        void Unlock();

    private:
        pthread_rwlock_t rw_;

        // No copying
        RWLock(const RWLock&);
        void operator=(const RWLock&);
    };

    class InodeMutex {
    public:
        InodeMutex() = default;

        ~InodeMutex() = default;

        void ReadLock(const _meta_key_t &key);

        void WriteLock(const _meta_key_t &key);

        void Unlock(const _meta_key_t &key);

    private:
        const static unsigned int NUM_ILOCK = 1024;
        const static unsigned int ILOCK_BASE = 0x3ff;
        RWLock ilock[NUM_ILOCK];

        //no copying
        InodeMutex(const InodeMutex &);
        void operator=(const InodeMutex &);
    };

}

#endif //KVFS_INODE_MUTEX_HPP
