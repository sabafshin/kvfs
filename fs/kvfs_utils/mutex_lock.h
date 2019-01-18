/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   mutex_lock.h
 */

#ifndef KVFS_MUTEX_LOCK_H
#define KVFS_MUTEX_LOCK_H

#include "mutex.h"
#include <atomic>
#include <memory>

namespace kvfs {
class MutexLock {
 public:
  explicit MutexLock() : mu_(new Mutex()) {
    this->mu_->Lock();
  }
  ~MutexLock() { this->mu_->Unlock(); }

 private:
  Mutex *const mu_;
  // No copying allowed
  MutexLock(const MutexLock &);
  void operator=(const MutexLock &);
};

//
// Acquire a ReadLock on the specified RWMutex.
// The Lock will be automatically released then the
// object goes out of scope.
//
class ReadLock {
 public:

  explicit ReadLock() : mu_(new RWMutex()) {
    this->mu_->ReadLock();
  }
  ~ReadLock() { this->mu_->ReadUnlock(); }

 private:
  RWMutex *const mu_;
  // No copying allowed
  ReadLock(const ReadLock &);
  void operator=(const ReadLock &);
};

//
// Automatically unlock a locked mutex when the object is destroyed
//
class ReadUnlock {
 public:
  explicit ReadUnlock() : mu_(new RWMutex()) {
    this->mu_->AssertHeld();
  }
  ~ReadUnlock() { mu_->ReadUnlock(); }

 private:
  RWMutex *const mu_;
  // No copying allowed
  ReadUnlock(const ReadUnlock &) = delete;
  ReadUnlock &operator=(const ReadUnlock &) = delete;
};

//
// Acquire a WriteLock on the specified RWMutex.
// The Lock will be automatically released then the
// object goes out of scope.
//
class WriteLock {
 public:
  explicit WriteLock() : mu_(new RWMutex()) {
    this->mu_->WriteLock();
  }
  ~WriteLock() { this->mu_->WriteUnlock(); }

 private:
  RWMutex *const mu_;
  // No copying allowed
  WriteLock(const WriteLock &);
  void operator=(const WriteLock &);
};

//
// SpinMutex has very low overhead for low-contention cases.  Method names
// are chosen so you can use std::unique_lock or std::lock_guard with it.
//
class SpinMutex {
 public:
  SpinMutex() : locked_(false) {}

  bool try_lock() {
    auto currently_locked = locked_.load(std::memory_order_relaxed);
    return !currently_locked &&
        locked_.compare_exchange_weak(currently_locked, true,
                                      std::memory_order_acquire,
                                      std::memory_order_relaxed);
  }

  void lock() {
    for (size_t tries = 0;; ++tries) {
      if (try_lock()) {
        // success
        break;
      }
      AsmVolatilePause();
      if (tries > 100) {
        std::this_thread::yield();
      }
    }
  }

  void unlock() { locked_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> locked_;
};
}  // namespace kvfs
#endif //KVFS_MUTEX_LOCK_H
