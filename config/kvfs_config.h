/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs-config.h
 */

#pragma once

#define KVFS_VERSION "0.1"

#ifndef KVFS_HAVE_ROCKSDB
#define KVFS_HAVE_ROCKSDB
#endif

//#define KVFS_DEBUG

/* #undef KVFS_HAVE_ROCKSDB */

#define CACHE_SIZE              512
#define KVFS_LINK_MAX           32000
#define KVFS_DEF_BLOCK_SIZE     4096
//#define KVFS_INODE_SIZE         64
//#define KVFS_MAX_NAME_SIZE      256
//#define KVFS_DEF_BLOCK_SIZE_2MB 2097152
#define KVFS_MAX_OPEN_FILES     512
//#define KVFS_PATH_DELIMITER     '/'

// flags
//#define KVFS_READ            0x00000001
//#define KVFS_WRITE           0x00000002
//#define KVFS_APPEND          0x00000004
//#define KVFS_CREAT           0x00000008
//#define KVFS_TRUNC           0x00000010
//#define KVFS_EXCL            0x00000020

// functions
//#define KVFS_INIT                1
//#define KVFS_OPEN                3
//#define KVFS_CLOSE               4
//#define KVFS_READ                5
//#define KVFS_WRITE               6
//#define KVFS_LSTAT               7
//#define KVFS_FSTAT               8
//#define KVFS_SETSTAT             9
//#define KVFS_FSETSTAT           10 /* NOT implemented */
//#define KVFS_OPENDIR            11
//#define KVFS_READDIR            12
//#define KVFS_REMOVE             13
//#define KVFS_MKDIR              14
//#define KVFS_RMDIR              15
//#define KVFS_REALPATH           16
//#define KVFS_STAT               17
//#define KVFS_RENAME             18
//#define KVFS_READLINK           19
//#define KVFS_SYMLINK            20
//#define KVFS_STATUS            101
//#define KVFS_HANDLE            102 /* NOT implemented */
//#define KVFS_DATA              103
//#define KVFS_NAME              104
//#define KVFS_ATTRS             105
//#define KVFS_EXTENDED          200 /* NOT implemented */
//#define KVFS_EXTENDED_REPLY    201 /* NOT implemented */