/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs-config.h.in
 */

#pragma once

#define KVFS_VERSION "0.1"

#ifndef KVFS_HAVE_ROCKSDB
#define KVFS_HAVE_ROCKSDB
#endif

/* #undef KVFS_HAVE_ROCKSDB */

#define CACHE_SIZE              512
#define KVFS_LINK_MAX           32000
#define KVFS_DEF_BLOCK_SIZE_4K  4096
#define KVFS_INODE_SIZE         64
#define KVFS_NAME_LEN           255