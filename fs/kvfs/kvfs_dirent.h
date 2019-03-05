/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_dirent.h
 */

#ifndef KVFS_KVFS_DIRENT_H
#define KVFS_KVFS_DIRENT_H

typedef size_t kvfs_file_hash_t;
typedef unsigned char byte;
#ifdef __USE_LARGEFILE64
typedef struct dirent64 kvfs_dirent;
typedef struct stat64 kvfs_stat;
typedef ino64_t kvfs_file_inode_t;
typedef off64_t kvfs_off_t;
#else
typedef struct dirent kvfs_dirent;
typedef ino_t kvfs_file_inode_t;
typedef struct stat kvfs_stat;
typedef off_t kvfs_off_t;
#endif

struct __kvfs_dir_stream {
  uint32_t file_descriptor_;
  kvfs_off_t offset_;
  std::string from_;
  uint64_t prefix;
};

/**
 * This stucture is to be used for dirstreams, the actual structure details are OPAQUE to the users.
 */
typedef __kvfs_dir_stream kvfsDIR;



#endif //KVFS_KVFS_DIRENT_H
