/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.hpp
 */

#ifndef KVFS_HPP
#define KVFS_HPP

#include <fuse.h>
#include <kvfs/kvfs_state.hpp>
#include <kvfs/inode_mutex.hpp>
#include <kvfs/dentry.hpp>
#include <kvfs/inode_cache.hpp>


namespace kvfs {

    class KVFS {
    public:
        ~KVFS() = default;

        void SetState(FileSystemState *state);

        void *Init(struct fuse_conn_info *conn);

        void Destroy(void *data);

        int GetAttr(StringPiece path, struct stat *statbuf);

        int Open(StringPiece path, struct fuse_file_info *fi);

        ssize_t Read(StringPiece path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

        ssize_t Write(StringPiece path, StringPiece buf, size_t size, off_t offset, struct fuse_file_info *fi);

        int Truncate(StringPiece path, size_t offset);

        int Fsync(StringPiece path, int datasync, struct fuse_file_info *fi);

        int Release(StringPiece path, struct fuse_file_info *fi);

        int Readlink(StringPiece path, char *buf, size_t size);

        int Symlink(StringPiece target, StringPiece path);

        int Unlink(StringPiece path);

        int MakeNode(StringPiece path, mode_t mode, dev_t dev);

        int MakeDir(StringPiece path, mode_t mode);

        int OpenDir(StringPiece path, struct fuse_file_info *fi);

        int ReadDir(StringPiece path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

        int ReleaseDir(StringPiece path, struct fuse_file_info *fi);

        int RemoveDir(StringPiece path);

        int Rename(StringPiece new_path, StringPiece old_path);

        int Access(StringPiece path, int mask);

        int UpdateTimens(StringPiece path, const struct timespec tv[2]);

        int Chmod(StringPiece path, mode_t mode);

        int Chown(StringPiece path, uid_t uid, gid_t gid);

        void Compact();

        bool GetStat(std::string stat, std::string *value);

    private:
        FileSystemState *state_;
        RocksDbStore *metadb;
        InodeCache  *inode_cache;
        DentryCache *dentry_cache;
        InodeMutex  fstree_lock;
        bool        flag_fuse_enabled;

        inline int FSError(const char *error_message);

        virtual inline void DeleteDBFile(kvfs_inode_t inode_id, int filesize) = 0;

        inline void GetDiskFilePath(char *path, kvfs_inode_t inode_id);

        inline int OpenDiskFile(const kvfs_inode_header *iheader, int flags);

        inline int TruncateDiskFile(kvfs_inode_t inode_id, off_t new_size);

        inline ssize_t MigrateDiskFileToBuffer(kvfs_inode_t inode_it, char *buffer, size_t size);

        int MigrateToDiskFile(InodeCacheHandle *handle, int &fd, int flags);

        inline void CloseDiskFile(int &fd_);

        inline void InitStat(struct stat &statbuf, kvfs_inode_t inode, mode_t mode, dev_t dev);

        _inode_val_t InitInodeValue(kvfs_inode_t inum, mode_t mode, dev_t dev, StringPiece filename);

        StringPiece InitInodeValue(StringPiece &old_value, StringPiece filename);

        void FreeInodeValue(_inode_val_t &ival);

        bool
        ParentPathLookup(StringPiece path, _meta_key_t &key, kvfs_inode_t &inode_in_search, StringPiece &lastdelimiter);

        inline bool PathLookup(StringPiece path, _meta_key_t &key, StringPiece &filename);

        inline bool PathLookup(StringPiece path, _meta_key_t &key);

    };

}

#endif //KVFS_HPP
