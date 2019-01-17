/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   mutex.h
 */

#ifndef KVFS_MUTEX_H
#define KVFS_MUTEX_H

#include <thread>

#define __declspec(S)

#undef PLATFORM_IS_LITTLE_ENDIAN

#include <endian.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <limits>
#include <string>
#include <assert.h>
#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <cstdlib>

#ifndef PLATFORM_IS_LITTLE_ENDIAN
#define PLATFORM_IS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
#endif

namespace kvfs {
static const bool kLittleEndian = PLATFORM_IS_LITTLE_ENDIAN;
#undef PLATFORM_IS_LITTLE_ENDIAN

class CondVar;

class Mutex {
 public:

  explicit Mutex(bool adaptive = false);
  ~Mutex();

  void Lock();
  void Unlock();
  // this will assert if the mutex is not locked
  // it does NOT verify that mutex is held by a calling thread
  void AssertHeld();

 private:
  friend class CondVar;
  pthread_mutex_t mu_;

  bool locked_;

  // No copying
  Mutex(const Mutex &);
  void operator=(const Mutex &);
};

class RWMutex {
 public:
  RWMutex();
  ~RWMutex();

  void ReadLock();
  void WriteLock();
  void ReadUnlock();
  void WriteUnlock();
  void AssertHeld() {}

 private:
  pthread_rwlock_t mu_; // the underlying platform mutex

  // No copying allowed
  RWMutex(const RWMutex &);
  void operator=(const RWMutex &);
};

class CondVar {
 public:
  explicit CondVar(Mutex *mu);
  ~CondVar();
  void Wait();
  // Timed condition wait.  Returns true if timeout occurred.
  bool TimedWait(uint64_t abs_time_us);
  void Signal();
  void SignalAll();
 private:
  pthread_cond_t cv_;
  Mutex *mu_;
};

using Thread = std::thread;

static inline void AsmVolatilePause() {
#if defined(__i386__) || defined(__x86_64__)
  asm volatile("pause");
#elif defined(__aarch64__)
  asm volatile("wfe");
#elif defined(__powerpc64__)
  asm volatile("or 27,27,27");
#endif
  // it's okay for other platforms to be no-ops
}
}  // namespace kvfs

#endif //KVFS_MUTEX_H
