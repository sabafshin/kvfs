/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_mutex.cpp
 */

#include "rocksdb_mutex.hpp"

kvfs::rocksdb_mutex::~rocksdb_mutex() {
  mutex.reset();
}

void kvfs::rocksdb_mutex::Lock() {
  mutex->Lock();
}

void kvfs::rocksdb_mutex::Unlock() {
  mutex->Unlock();
}

void kvfs::rocksdb_mutex::AssertHeld() {
  mutex->AssertHeld();
}
kvfs::rocksdb_mutex::rocksdb_mutex(bool adaptive) : mutex(std::make_unique<rocksdb::port::Mutex>(adaptive)) {}

//------------------------------------------------------------------//

kvfs::rocksdb_rw_mutex::rocksdb_rw_mutex() : rwMutex(std::make_unique<rocksdb::port::RWMutex>()) {}

kvfs::rocksdb_rw_mutex::~rocksdb_rw_mutex() {
  rwMutex.reset();
}

void kvfs::rocksdb_rw_mutex::ReadLock() {
  rwMutex->ReadLock();
}

void kvfs::rocksdb_rw_mutex::WriteLock() {
  rwMutex->WriteLock();
}

void kvfs::rocksdb_rw_mutex::ReadUnlock() {
  rwMutex->ReadUnlock();
}

void kvfs::rocksdb_rw_mutex::WriteUnlock() {
  rwMutex->WriteUnlock();
}

void kvfs::rocksdb_rw_mutex::AssertHeld() {
  rwMutex->AssertHeld();
}











