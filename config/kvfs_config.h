/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_config.h
 */

#pragma once

#if !defined(KVFS_VERSION)
#define KVFS_VERSION "1.0"
#endif  // !defined(KVFS_VERSION)
#if !defined(KVFS_HAVE_ROCKSDB)
#define KVFS_HAVE_ROCKSDB 1
#endif  // !defined(KVFS_HAVE_ROCKSDB)
#if !defined(KVFS_HAVE_LEVELDB)
#define KVFS_HAVE_LEVELDB 0
#endif  // !defined(KVFS_HAVE_LEVELDB)

#if !defined(KVFS_DEF_BLOCK_SIZE)
#define KVFS_DEF_BLOCK_SIZE 1024
#endif  // !defined(KVFS_DEF_BLOCK_SIZE)

#if !defined(KVFS_MAX_OPEN_FILES)
#define KVFS_MAX_OPEN_FILES 512
#endif  // !defined(KVFS_DEF_BLOCK_SIZE)

#if !defined(KVFS_CACHE_SIZE)
#define KVFS_CACHE_SIZE 512
#endif  // !defined(KVFS_CACHE_SIZE)

#if !defined(KVFS_LINK_MAX)
#define KVFS_LINK_MAX 1000
#endif  // !defined(KVFS_LINK_MAX)

#if !defined(KVFS_THREAD_SAFE)
#define KVFS_THREAD_SAFE  0
#endif  // !defined(KVFS_THREAD_SAFE)
