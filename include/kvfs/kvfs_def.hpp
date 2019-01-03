/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs_def.hpp
 */

#ifndef _KVFS_DEF_HPP
#define _KVFS_DEF_HPP

/**
 * @brief Definitions for kvfs Filesystem
 */

#include <linux/types.h>
#include <linux/magic.h>

/*
 * The KVFS filesystem constants/structures
 */

/*
 * Mount flags
 */
#define KVFS_MOUNT_PROTECT      0x000001    /* wprotect CR0.WP */
#define KVFS_MOUNT_XATTR_USER   0x000002    /* Extended user attributes */
#define KVFS_MOUNT_POSIX_ACL    0x000004    /* POSIX Access Control Lists */
#define KVFS_MOUNT_DAX          0x000008    /* Direct Access */
#define KVFS_MOUNT_ERRORS_CONT  0x000010    /* Continue on errors */
#define KVFS_MOUNT_ERRORS_RO    0x000020    /* Remount fs ro on errors */
#define KVFS_MOUNT_ERRORS_PANIC 0x000040    /* Panic on errors */
#define KVFS_MOUNT_HUGEMMAP     0x000080    /* Huge mappings with mmap */
#define KVFS_MOUNT_HUGEIOREMAP  0x000100    /* Huge mappings with ioremap */
#define KVFS_MOUNT_FORMAT       0x000200    /* was FS formatted on mount? */
#define KVFS_MOUNT_DATA_COW 0x000400 /* Copy-on-write for data integrity */


/*
 * Maximal count of links to a file
 */
#define KVFS_LINK_MAX          32000

#define KVFS_DEF_BLOCK_SIZE_4K 4096

#define KVFS_INODE_BITS   7
#define KVFS_INODE_SIZE   128    /* must be power of two */

#define KVFS_NAME_LEN 255

#define MAX_CPUS 1024

/* KVFS supported data blocks */
#define KVFS_BLOCK_TYPE_4K     0
#define KVFS_BLOCK_TYPE_2M     1
#define KVFS_BLOCK_TYPE_1G     2
#define KVFS_BLOCK_TYPE_MAX    3

#define META_BLK_SHIFT 9

#define KVFS_DEFAULT_BLOCK_TYPE KVFS_BLOCK_TYPE_4K

/*
 * nova inode flags
 *
 * KVFS_EOFBLOCKS_FL	There are blocks allocated beyond eof
 */
#define KVFS_EOFBLOCKS_FL      0x20000000
/* Flags that should be inherited by new inodes from their parent. */
#define KVFS_FL_INHERITED (FS_SECRM_FL | FS_UNRM_FL | FS_COMPR_FL | \
			    FS_SYNC_FL | FS_NODUMP_FL | FS_NOATIME_FL |	\
			    FS_COMPRBLK_FL | FS_NOCOMP_FL | \
			    FS_JOURNAL_DATA_FL | FS_NOTAIL_FL | FS_DIRSYNC_FL)
/* Flags that are appropriate for regular files (all but dir-specific ones). */
#define KVFS_REG_FLMASK (~(FS_DIRSYNC_FL | FS_TOPDIR_FL))
/* Flags that are appropriate for non-directories/regular files. */
#define KVFS_OTHER_FLMASK (FS_NODUMP_FL | FS_NOATIME_FL)
#define KVFS_FL_USER_VISIBLE (FS_FL_USER_VISIBLE | KVFS_EOFBLOCKS_FL)

/* IOCTLs */
#define	KVFS_PRINT_TIMING		0xBCD00010
#define	KVFS_CLEAR_STATS		0xBCD00011
#define	KVFS_PRINT_LOG			0xBCD00013
#define	KVFS_PRINT_LOG_BLOCKNODE	0xBCD00014
#define	KVFS_PRINT_LOG_PAGES		0xBCD00015
#define	KVFS_PRINT_FREE_LISTS	0xBCD00018

#endif //_KVFS_DEF_HPP
