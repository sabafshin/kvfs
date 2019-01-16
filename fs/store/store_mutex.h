/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_mutex.h
 */

#ifndef KVFS_STORE_MUTEX_H
#define KVFS_STORE_MUTEX_H

#include <thread>

namespace kvfs {
class store_mutex {
 public:

  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  // this will assert if the mutex is not locked
  // it does NOT verify that mutex is held by a calling thread
  virtual void AssertHeld() = 0;
/*
 private:
  // No copying allowed
  store_mutex(const store_mutex &);
  virtual void operator=(const store_mutex &);*/
};

class store_rw_mutex {
 public:
  virtual void ReadLock() = 0;
  virtual void WriteLock() = 0;
  virtual void ReadUnlock() = 0;
  virtual void WriteUnlock() = 0;
  virtual void AssertHeld() = 0;
};
}  // namespace kvfs
#endif //KVFS_STORE_MUTEX_H
