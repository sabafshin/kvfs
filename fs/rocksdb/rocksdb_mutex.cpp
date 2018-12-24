/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_mutex.cpp
 */

#include "rocksdb_mutex.hpp"

kvfs::Mutex::~Mutex() {
    mutex.reset();
}

void kvfs::Mutex::Lock() {
    mutex->Lock();
}

void kvfs::Mutex::Unlock() {
    mutex->Unlock();
}

void kvfs::Mutex::AssertHeld() {
    mutex->AssertHeld();
}


kvfs::RWMutex::RWMutex() = default;

kvfs::RWMutex::~RWMutex() {
    rwMutex.reset();
}

void kvfs::RWMutex::ReadLock() {
    rwMutex->ReadLock();
}

void kvfs::RWMutex::WriteLock() {
    rwMutex->WriteLock();
}

void kvfs::RWMutex::ReadUnlock() {
    rwMutex->ReadUnlock();
}

void kvfs::RWMutex::WriteUnlock() {
    rwMutex->WriteUnlock();
}

void kvfs::RWMutex::AssertHeld() {
    rwMutex->AssertHeld();
}











