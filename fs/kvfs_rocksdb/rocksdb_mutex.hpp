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

#define ROCKSDB_PTHREAD_ADAPTIVE_MUTEX
#include <port/port_posix.h>
#include "../store/store_mutex.h"

namespace kvfs {
class rocksdb_mutex : public store_mutex {
 public:
  explicit rocksdb_mutex(bool adaptive = true);
  ~rocksdb_mutex();

  void Lock() override;
  void Unlock() override;
  // this will assert if the mutex is not locked
  // it does NOT verify that mutex is held by a calling thread
  void AssertHeld() override;

 private:
  std::unique_ptr<rocksdb::port::Mutex> mutex;

  // No copying allowed
  rocksdb_mutex(const rocksdb_mutex &);
  void operator=(const rocksdb_mutex &);
};

class rocksdb_rw_mutex : public store_rw_mutex {
 public:
  rocksdb_rw_mutex();
  ~rocksdb_rw_mutex();

  void ReadLock() override;
  void WriteLock() override;
  void ReadUnlock() override;
  void WriteUnlock() override;
  void AssertHeld() override;

 private:
  std::unique_ptr<rocksdb::port::RWMutex> rwMutex; // the underlying platform mutex

  // No copying allowed
  rocksdb_rw_mutex(const rocksdb_rw_mutex &);
  void operator=(const rocksdb_rw_mutex &);
};
}  // namespace kvfs


#endif //KVFS_ROCKSDB_ROCKSDB_MUTEX_HPP
