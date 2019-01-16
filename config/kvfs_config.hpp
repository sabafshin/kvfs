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

#define CACHE_SIZE 512
/* #undef KVFS_HAVE_ROCKSDB */
