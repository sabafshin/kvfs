/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_cache.h
 */

#ifndef KVFS_STORE_CACHE_H
#define KVFS_STORE_CACHE_H

#include "slice.hpp"
#include "store_result.hpp"

namespace kvfs {
class store_cache {
  /**
   * Insert a mapping from key->value into the cache
   * @param key
   * @param value
   * @return true if successfull
   */
  virtual bool insert(const slice &key, const slice &value) = 0;
  /**
   * If the cache has no mapping for "key", returns nullptr.
   * @param key
   * @return a handle that corresponds to the mapping
   */
  virtual StoreResult lookup(const slice &key) = 0;
  /**
   * Release a mapping returned by a previous Lookup().
   * @param handle
   * @param force_erase
   * @return Returns true if the entry was also erased.
   */
  virtual bool release(const StoreResult &handle, bool force_erase) = 0;
  /**
   * If the cache contains entry for key, erase it.
   * @param key
   * @return true on success
   */
  virtual bool erase(const slice &key) = 0;
  /**
   * Remove all entries.
   * Prerequisite: no entry is referenced.
   */
  virtual void erase_un_ref_entries() = 0;

  virtual void clean_handle(const slice &key, void *value) = 0;
};
}  // namespace kvfs
#endif //KVFS_STORE_CACHE_H
