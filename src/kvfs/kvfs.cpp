/*
 * Copyright (c) 2018 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.cpp
 */

#include <kvfs/kvfs.hpp>

#include <util/crc32c.h>
#include <util/xxhash.h>
#include <folly/Conv.h>

using namespace rocksdb::crc32c;


namespace kvfs {

    struct kvfs_file_handle_t {
        InodeCacheHandle *handle_;
        int              flags_{};
        int              fd_;

        kvfs_file_handle_t() : handle_(nullptr), fd_(-1) {}
    };

    inline static void BuildMetaKey(const kvfs_inode_t inode_id, const kvfs_hash_t hash_id, _meta_key_t &key) {
        key.inode_id = inode_id;
        key.hash_id  = hash_id;
    }

    inline static void BuildMetaKey(StringPiece path, const int len, const kvfs_inode_t inode_id, _meta_key_t &key) {
        BuildMetaKey(inode_id, rocksdb::XXH32(path.data(), len, 123), key);
    }

    inline static bool IsKeyInDir(const rocksdb::Slice &key, const _meta_key_t &dirkey) {
        const auto *rkey = (const _meta_key_t *) key.data();
        return rkey->inode_id == dirkey.inode_id;
    }

    const kvfs_inode_header *GetInodeHeader(StringPiece value) {
        return reinterpret_cast<const kvfs_inode_header *> (value.data());
    }

    const kvfs_stat_t *GetAttribute_store(StoreResult &value) {
        return reinterpret_cast<const kvfs_stat_t *> (value.piece().data());
    }

    const kvfs_stat_t *GetAttribute_str(StringPiece &value) {
        return reinterpret_cast<const kvfs_stat_t *> (value.data());
    }

    size_t GetInlineData(StringPiece value, char *buf, off_t offset, size_t size) {
        const kvfs_inode_header *header    = GetInodeHeader(value);
        size_t                  realoffset = KVFS_INODE_HEADER_SIZE + header->namelen + 1 + offset;
        if (realoffset < value.size()) {
            if (realoffset + size > value.size()) {
                size = value.size() - realoffset;
            }
            memcpy(buf, value.data() + realoffset, size);
            return size;
        }
        else {
            return 0;
        }
    }

    // TODO: Fix this stupid copy string value
    void UpdateIhandleValue(StringPiece &value, StringPiece buf, size_t offset, size_t size) {
        if (offset > value.size()) {
            value.toString().resize(offset);
        }
        value.toString().replace(offset, size, buf.data(), size);
    }

    void UpdateInodeHeader(StringPiece &value, kvfs_inode_header &new_header) {
        UpdateIhandleValue(value, (const char *) &new_header, 0, KVFS_INODE_HEADER_SIZE);
    }

    void UpdateAttribute(StringPiece &value, const kvfs_stat_t &new_fstat) {
        UpdateIhandleValue(value, (const char *) &new_fstat, 0, KVFS_INODE_ATTR_SIZE);
    }

    void UpdateInlineData(StringPiece &value, StringPiece buf, off_t offset, size_t size) {
        const kvfs_inode_header *header    = GetInodeHeader(value);
        size_t                  realoffset = KVFS_INODE_HEADER_SIZE + header->namelen + 1 + offset;
        UpdateIhandleValue(value, buf, realoffset, size);
    }

    void TruncateInlineData(StringPiece &value, size_t new_size) {
        const kvfs_inode_header *header     = GetInodeHeader(value);
        size_t                  target_size = KVFS_INODE_HEADER_SIZE + header->namelen + new_size + 1;
        value.toString().resize(target_size);
    }

    void DropInlineData(StringPiece &value) {
        const kvfs_inode_header *header     = GetInodeHeader(value);
        size_t                  target_size = KVFS_INODE_HEADER_SIZE + header->namelen + 1;
        value.toString().resize(target_size);
    }

    void KVFS::SetState(FileSystemState *state) {
        state_ = state;
    }

    int KVFS::FSError(const char *err_msg) {
        int retv = -errno;
#ifdef KVFS_DEBUG
        state_->GetLog()->LogMsg(err_msg);
#endif
        return retv;
    }

    void KVFS::InitStat(kvfs_stat_t &statbuf, kvfs_inode_t inode, mode_t mode, dev_t dev) {
        statbuf.st_ino  = inode;
        statbuf.st_mode = mode;
        statbuf.st_dev  = dev;

        if (flag_fuse_enabled) {
//            statbuf.st_gid = fuse_get_context()->gid;
//            statbuf.st_uid = fuse_get_context()->uid;
        }
        else {
            statbuf.st_gid = 0;
            statbuf.st_uid = 0;
        }

        statbuf.st_size    = 0;
        statbuf.st_blksize = 0;
        statbuf.st_blocks  = 0;
        if S_ISREG(mode) {
            statbuf.st_nlink = 1;
        }
        else {
            statbuf.st_nlink = 2;
        }
        time_t now = time(nullptr);
        statbuf.st_atim.tv_nsec = 0;
        statbuf.st_mtim.tv_nsec = 0;
        statbuf.st_ctim.tv_sec  = now;
        statbuf.st_ctim.tv_nsec = 0;
    }

    _inode_val_t KVFS::InitInodeValue(kvfs_inode_t inum, mode_t mode, dev_t dev, StringPiece filename) {
        _inode_val_t ival;
        ival.size  = KVFS_INODE_HEADER_SIZE + filename.size() + 1;
        ival.value = new char[ival.size];
        auto *header = reinterpret_cast<kvfs_inode_header *>(ival.value);
        InitStat(header->fstat, inum, mode, dev);
        header->has_blob = 0;
        header->namelen  = filename.size();
        char *name_buffer = ival.value + KVFS_INODE_HEADER_SIZE;
        memcpy(name_buffer, filename.data(), filename.size());
        name_buffer[header->namelen] = '\0';
        return ival;
    }

    StringPiece KVFS::InitInodeValue(StringPiece &old_value, StringPiece filename) {
        StringPiece       new_value = old_value;
        kvfs_inode_header header    = *GetInodeHeader(old_value);
        new_value.toString().replace(KVFS_INODE_HEADER_SIZE, header.namelen + 1, filename.data(), filename.size() + 1);
        header.namelen = filename.size();
        UpdateInodeHeader(new_value, header);

        return new_value;
    }

    void KVFS::FreeInodeValue(_inode_val_t &ival) {
        if (ival.value != nullptr) {
            delete[] ival.value;
            ival.value = nullptr;
        }
    }

    bool KVFS::ParentPathLookup(StringPiece path, _meta_key_t &key, kvfs_inode_t &inode_in_search
                                , StringPiece &lastdelimiter) {
        const char  *lpos      = path.data();
        const char  *rpos;
        bool        flag_found = true;
        std::string item;
        inode_in_search = ROOT_INODE_ID;
        while ((rpos    = strchr(lpos + 1, PATH_DELIMITER)) != nullptr) {
            if (rpos - lpos > 0) {
                BuildMetaKey(StringPiece(lpos + 1), static_cast<const int>(rpos - lpos - 1), inode_in_search, key);
                if (!dentry_cache->Find(key, inode_in_search)) {
                    {
                        fstree_lock.ReadLock(key);
                        StoreResult result = metadb->get(key.ToByteRange());
                        if (result.isValid()) {
                            inode_in_search = GetAttribute_store(result)->st_ino;
                            dentry_cache->Insert(key, inode_in_search);
                        }
                        else {
                            errno = ENOENT;
                            flag_found = false;
                        }
                        fstree_lock.Unlock(key);
                        if (!flag_found) {
                            return false;
                        }
                    }
                }
            }
            lpos = rpos;
        }
        if (lpos == path.data()) {
            BuildMetaKey("", 0, ROOT_INODE_ID, key);
        }
        lastdelimiter = folly::Range(lpos);
        return flag_found;
    }

    bool KVFS::PathLookup(StringPiece path, _meta_key_t &key) {
        const char   *lpos;
        kvfs_inode_t inode_in_search;
        if (ParentPathLookup(path, key, inode_in_search, (StringPiece &) lpos)) {
            const char *rpos = strchr(lpos, '\0');
            if (rpos != nullptr && rpos - lpos > 1) {
                BuildMetaKey(StringPiece(lpos + 1), static_cast<const int>(rpos - lpos - 1), inode_in_search, key);
            }
            return true;
        }
        else {
            errno = ENOENT;
            return false;
        }
    }

    bool KVFS::PathLookup(StringPiece path, _meta_key_t &key, StringPiece &filename) {
        const char   *lpos;
        kvfs_inode_t inode_in_search;
        if (ParentPathLookup(path, key, inode_in_search, (StringPiece &) lpos)) {
            const char *rpos = strchr(lpos, '\0');
            if (rpos != nullptr && rpos - lpos > 1) {
                BuildMetaKey(StringPiece(lpos + 1), static_cast<const int>(rpos - lpos - 1), inode_in_search, key);
                filename = StringPiece(lpos + 1, rpos - lpos - 1);
            }
            else {
                filename = StringPiece(lpos, 1);
            }
            return true;
        }
        else {
            errno = ENOENT;
            return false;
        }
    }

    void *KVFS::Init(struct fuse_conn_info *conn) {
//        XLOG(INFO) << "KVFS initialized.\n";
        flag_fuse_enabled = conn != nullptr;
        metadb            = state_->GetMetaDB();
        if (state_->IsEmpty()) {
//            XLOG(INFO) << "KVFS create root inode.\n";
            _meta_key_t key{};
            BuildMetaKey("", 0, ROOT_INODE_ID, key);
            struct stat statbuf{};
            lstat(ROOT_INODE_STAT, &statbuf);
            _inode_val_t value = InitInodeValue(ROOT_INODE_ID, statbuf.st_mode, statbuf.st_dev, folly::fbstring("\0"));
            if (metadb->put(key.ToByteRange(), value.ToByteRange())) {
//                XLOG(INFO) << "KVFS create root directory failed.\n";
            }
            FreeInodeValue(value);
        }
        inode_cache  = new InodeCache(metadb);
        dentry_cache = new DentryCache(16384);
        return state_;
    }

    void KVFS::Destroy(void *data) {

        delete dentry_cache;

        delete inode_cache;

//        XLOG(INFO, "Clean write back cache.\n");
        state_->Destroy();
        delete state_;
    }

    int KVFS::GetAttr(StringPiece path, struct stat *statbuf) {
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("GetAttr Path Lookup: No such file or directory: %s\n");
        }
        int ret = 0;
        fstree_lock.ReadLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_READ);
        if (handle != nullptr) {
            *statbuf = *(GetAttribute_str(handle->value_));
#ifdef KVFS_DEBUG
            state_->GetLog()->LogMsg("GetAttr: %s, Handle: %x HandleMode: %d\n",
                            path, handle, handle->mode_);
  state_->GetLog()->LogMsg("GetAttr DBKey: [%d,%d]\n", key.inode_id, key.hash_id);
  state_->GetLog()->LogStat(path, statbuf);
#endif
            inode_cache->Release(handle);
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);
        return ret;
    }

    void KVFS::GetDiskFilePath(char *path, kvfs_inode_t inode_id) {
        sprintf(path, "%s/%d/%d", state_->GetDataDir().data(), (int) inode_id >> NUM_FILES_IN_DATADIR_BITS,
                (int) inode_id % (NUM_FILES_IN_DATADIR));
    }

    int KVFS::OpenDiskFile(const kvfs_inode_header *iheader, int flags) {
        char fpath[128];
        GetDiskFilePath(fpath, iheader->fstat.st_ino);
        int fd = open(fpath, flags | O_CREAT, iheader->fstat.st_mode);
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("OpenDiskFile: %s InodeID: %d FD: %d\n",
                           fpath, iheader->fstat.st_ino, fd);
#endif
        return fd;
    }

    int KVFS::TruncateDiskFile(kvfs_inode_t inode_id, off_t new_size) {
        char fpath[128];
        GetDiskFilePath(fpath, inode_id);
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("TruncateDiskFile: %s, InodeID: %d, NewSize: %d\n",
                          fpath, inode_id, new_size);
#endif
        return truncate(fpath, new_size);
    }

    ssize_t KVFS::MigrateDiskFileToBuffer(kvfs_inode_t inode_id, char *buffer, size_t size) {
        char fpath[128];
        GetDiskFilePath(fpath, inode_id);
        int     fd  = open(fpath, O_RDONLY);
        ssize_t ret = pread(fd, buffer, size, 0);
        close(fd);
        unlink(fpath);
        return ret;
    }

    int KVFS::MigrateToDiskFile(InodeCacheHandle *handle, int &fd, int flags) {
        const kvfs_inode_header *iheader = GetInodeHeader(handle->value_);
        if (fd >= 0) {
            close(fd);
        }
        fd = OpenDiskFile(iheader, flags);
        if (fd < 0) {
            fd = -1;
            return -errno;
        }
        int ret = 0;
        if (iheader->fstat.st_size > 0) {
            if (pwrite(fd, ((const char *) iheader +
                            (KVFS_INODE_HEADER_SIZE + iheader->namelen + 1))
                       , static_cast<size_t>(iheader->fstat.st_size), 0) !=
                iheader->fstat.st_size) {
                ret = -errno;
            }
            DropInlineData(handle->value_);
        }
        return ret;
    }

    void KVFS::CloseDiskFile(int &fd_) {
        close(fd_);
        fd_ = -1;
    }

    int KVFS::Open(StringPiece path, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Open: %s, Flags: %d\n", path, fi->flags);
#endif

        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("Open: No such file or directory\n");
        }
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = nullptr;
        if ((fi->flags & O_RDWR) > 0 ||
            (fi->flags & O_WRONLY) > 0 ||
            (fi->flags & O_TRUNC) > 0) {
            handle = inode_cache->Get(key, INODE_WRITE);
        }
        else {
            handle = inode_cache->Get(key, INODE_READ);
        }

        int ret = 0;
        if (handle != nullptr) {
            auto *fh = new kvfs_file_handle_t();
            fh->handle_ = handle;
            fh->flags_  = fi->flags;
            const kvfs_inode_header *iheader = GetInodeHeader(handle->value_);

            if (iheader->has_blob > 0) {
                fh->flags_ = fi->flags;
                fh->fd_    = OpenDiskFile(iheader, fh->flags_);
                if (fh->fd_ < 0) {
                    inode_cache->Release(handle);
                    ret = -errno;
                }
            }
#ifdef  KVFS_DEBUG
            state_->GetLog()->LogMsg("Open: %s, Handle: %x, HandleMode: %d FD: %d\n",
                           path, handle, handle->mode_, fh->fd_);
#endif
            if (ret == 0) {
                fi->fh = (uint64_t) fh;
            }
            else {
                delete fh;
            }
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);


        return ret;
    }

    ssize_t KVFS::Read(StringPiece path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Read: %s\n", path);
#endif
        auto             *fh     = reinterpret_cast<kvfs_file_handle_t *>(fi->fh);
        InodeCacheHandle *handle = fh->handle_;
        fstree_lock.ReadLock(handle->key_);
        const kvfs_inode_header *iheader = GetInodeHeader(handle->value_);
        ssize_t                 ret      = 0;
        if (iheader->has_blob > 0) {
            if (fh->fd_ < 0) {
                fh->fd_ = OpenDiskFile(iheader, fh->flags_);
                if (fh->fd_ < 0) {
                    ret = -EBADF;
                }
            }
            if (fh->fd_ >= 0) {
                ret = pread(fh->fd_, buf, size, offset);
            }
        }
        else {
            ret = GetInlineData(handle->value_, buf, offset, size);
        }
        fstree_lock.Unlock(handle->key_);
        return ret;
    }

    ssize_t KVFS::Write(StringPiece path, StringPiece buf, size_t size, off_t offset, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Write: %s %lld %d\n", path, offset, size);
#endif

        auto             *fh     = reinterpret_cast<kvfs_file_handle_t *>(fi->fh);
        InodeCacheHandle *handle = fh->handle_;
        handle->mode_ = INODE_WRITE;
        fstree_lock.ReadLock(handle->key_);
        const kvfs_inode_header *iheader        = GetInodeHeader(handle->value_);
        int                     has_imgrated    = 0;
        ssize_t                 ret             = 0;
        int                     has_larger_size = (iheader->fstat.st_size < offset + size) ? 1 : 0;

#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Write: %s has_larger_size %d old: %d new: %lld\n",
      path, has_larger_size, iheader->fstat.st_size, offset + size);
#endif

        if (iheader->has_blob > 0) {
            if (fh->fd_ < 0) {
                fh->fd_ = OpenDiskFile(iheader, fh->flags_);
                if (fh->fd_ < 0) {
                    ret = -EBADF;
                }
            }
            if (fh->fd_ >= 0) {
                ret = pwrite(fh->fd_, buf.data(), size, offset);
            }
        }
        else {
            if (offset + size > state_->GetThreshold()) {
                auto cursize = static_cast<size_t>(iheader->fstat.st_size);
                ret = MigrateToDiskFile(handle, fh->fd_, fi->flags);
                if (ret == 0) {
                    kvfs_inode_header new_iheader = *GetInodeHeader(handle->value_);
                    new_iheader.fstat.st_size = offset + size;
                    new_iheader.has_blob      = true;
                    UpdateInodeHeader(handle->value_, new_iheader);
#ifdef  KVFS_DEBUG
                    state_->GetLog()->LogMsg("Write: UpdateInodeHeader %d\n", GetInodeHeader(handle->value_)->has_blob);
#endif
                    has_imgrated = 1;
                    ret          = pwrite(fh->fd_, buf.data(), size, offset);
                }
            }
            else {
                UpdateInlineData(handle->value_, buf, static_cast<size_t>(offset), size);
                ret = size;
            }
        }
        if (ret >= 0) {
            if (has_larger_size > 0 && has_imgrated == 0) {
                kvfs_inode_header new_iheader = *GetInodeHeader(handle->value_);
                new_iheader.fstat.st_size = offset + size;
                UpdateInodeHeader(handle->value_, new_iheader);
            }
        }
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Write: %s, Handle: %x HandleMode: %d\n",
                           path, handle, handle->mode_);
#endif
        fstree_lock.Unlock(handle->key_);
        return ret;
    }

    int KVFS::Fsync(StringPiece path, int datasync, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Fsync: %s\n", path);
#endif

        auto             *fh     = reinterpret_cast<kvfs_file_handle_t *>(fi->fh);
        InodeCacheHandle *handle = fh->handle_;
        fstree_lock.WriteLock(handle->key_);

        const kvfs_inode_header *iheader = GetInodeHeader(handle->value_);
        int                     ret      = 0;
        if (handle->mode_ == INODE_WRITE) {
            if (iheader->has_blob > 0) {
                ret = fsync(fh->fd_);
            }
            if (datasync == 0) {
                ret = metadb->Sync();
            }
        }

        fstree_lock.Unlock(handle->key_);
        return -ret;
    }

    int KVFS::Release(StringPiece path, struct fuse_file_info *fi) {
        auto             *fh     = reinterpret_cast<kvfs_file_handle_t *>(fi->fh);
        InodeCacheHandle *handle = fh->handle_;
        fstree_lock.WriteLock(handle->key_);

        if (handle->mode_ == INODE_WRITE) {
            const kvfs_stat_t *value    = GetAttribute_str(handle->value_);
            kvfs_stat_t       new_value = *value;
            new_value.st_atim.tv_sec  = time(nullptr);
            new_value.st_atim.tv_nsec = 0;
            new_value.st_mtim.tv_sec  = time(nullptr);
            new_value.st_mtim.tv_nsec = 0;
            UpdateAttribute(handle->value_, new_value);
        }

#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Release: %s, FD: %d\n",
                           path, fh->fd_);
#endif

        int ret = 0;
        if (fh->fd_ != -1) {
            ret = close(fh->fd_);
        }
        inode_cache->WriteBack(handle);
        _meta_key_t key = handle->key_;
        inode_cache->Release(handle);
        inode_cache->Evict(key);
        delete fh;

        fstree_lock.Unlock(key);
        if (ret != 0) {
            return -errno;
        }
        else {
            return 0;
        }
    }

    int KVFS::Truncate(StringPiece path, size_t new_size) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Truncate: %s\n", path);
#endif

        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("Open: No such file or directory\n");
        }
        InodeCacheHandle *handle =
                                 inode_cache->Get(key, INODE_WRITE);
        fstree_lock.WriteLock(key);

        int ret = 0;
        if (handle != nullptr) {
            const kvfs_inode_header *iheader = GetInodeHeader(handle->value_);

            if (iheader->has_blob > 0) {
                if (new_size > state_->GetThreshold()) {
                    TruncateDiskFile(iheader->fstat.st_ino, new_size);
                }
                else {
                    char *buffer = new char[new_size];
                    MigrateDiskFileToBuffer(iheader->fstat.st_ino, buffer, new_size);
                    UpdateInlineData(handle->value_, buffer, 0, new_size);
                    delete[] buffer;
                }
            }
            else {
                if (new_size > state_->GetThreshold()) {
                    int fd;
                    if (MigrateToDiskFile(handle, fd, O_TRUNC | O_WRONLY) == 0) {
                        if ((ret = ftruncate(fd, new_size)) == 0) {
                            fsync(fd);
                        }
                        close(fd);
                    }
                }
                else {
                    TruncateInlineData(handle->value_, new_size);
                }
            }
            if (new_size != iheader->fstat.st_size) {
                kvfs_inode_header new_iheader = *GetInodeHeader(handle->value_);
                new_iheader.fstat.st_size = new_size;
                new_iheader.has_blob      = new_size > state_->GetThreshold();
                UpdateInodeHeader(handle->value_, new_iheader);
            }
            inode_cache->WriteBack(handle);
            inode_cache->Release(handle);
        }
        else {
            ret = -ENOENT;
        }

        fstree_lock.Unlock(key);
        return ret;
    }

    int KVFS::Readlink(StringPiece path, char *buf, size_t size) {
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("Open: No such file or directory\n");
        }
        fstree_lock.ReadLock(key);
        StoreResult result = metadb->get(key.ToString());
        int         ret    = 0;
        if (result.isValid()) {
            size_t data_size = GetInlineData(result.piece(), buf, 0, size - 1);
            buf[data_size] = '\0';
        }
        else {
            errno = ENOENT;
            ret = -1;
        }
        fstree_lock.Unlock(key);
        if (ret < 0) {
            return FSError("Open: No such file or directory\n");
        }
        else {
            return 0;
        }
    }

    int KVFS::Symlink(StringPiece target, StringPiece path) {
        _meta_key_t key{};
        StringPiece filename;
        if (!PathLookup(path, key, filename)) {
#ifdef  KVFS_DEBUG
            state_->GetLog()->LogMsg("Symlink: %s %s\n", path, target);
#endif
            return FSError("Symlink: No such parent file or directory\n");
        }

        size_t val_size = KVFS_INODE_HEADER_SIZE + filename.size() + 1 + target.size();
        char   *value   = new char[val_size];
        auto   *header  = reinterpret_cast<kvfs_inode_header *>(value);
        InitStat(header->fstat, state_->NewInode(), S_IFLNK, 0);
        header->has_blob = false;
        header->namelen  = filename.size();
        char *name_buffer = value + KVFS_INODE_HEADER_SIZE;
        memcpy(name_buffer, filename.data(), filename.size());
        name_buffer[header->namelen] = '\0';
        strncpy(name_buffer + filename.size() + 1, target.data(), target.size());

        fstree_lock.WriteLock(key);
        metadb->put(key.ToByteRange(), folly::Range(value, val_size));
        fstree_lock.Unlock(key);
        delete[] value;
        return 0;
    }

    int KVFS::Unlink(StringPiece path) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Unlink: %s\n", path);
#endif
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("Unlink: No such file or directory\n");
        }

        int ret = 0;
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_DELETE);
        if (handle != nullptr) {
            const kvfs_inode_header *value = GetInodeHeader(handle->value_);
            if (value->fstat.st_size > state_->GetThreshold()) {
                char fpath[128];
                GetDiskFilePath(fpath, value->fstat.st_ino);
                unlink(fpath);
            }
            dentry_cache->Evict(key);
            inode_cache->Release(handle);
            inode_cache->Evict(key);
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);

        return ret;
    }

    int KVFS::MakeNode(StringPiece path, mode_t mode, dev_t dev) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("MakeNode: %s\n", path);
#endif
        _meta_key_t key{};
        StringPiece filename;
        if (!PathLookup(path, key, filename)) {
            return FSError("MakeNode: No such parent file or directory\n");
        }

        _inode_val_t value =
                             InitInodeValue(state_->NewInode(), mode | S_IFREG, dev, filename);

        int ret = 0;
        {
            fstree_lock.WriteLock(key);
            ret = metadb->put(key.ToByteRange(), value.ToByteRange());
            fstree_lock.Unlock(key);
        }

        FreeInodeValue(value);

        if (ret == 0) {
            return 0;
        }
        else {
            errno = ENOENT;
            return FSError("MakeNode failed\n");
        }
    }

    int KVFS::MakeDir(StringPiece path, mode_t mode) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("MakeDir: %s\n", path);
#endif
        _meta_key_t key{};
        StringPiece filename;
        if (!PathLookup(path, key, filename)) {
            return FSError("MakeDir: No such parent file or directory\n");
        }

        _inode_val_t value =
                             InitInodeValue(state_->NewInode(), mode | S_IFDIR, 0, filename);

        int ret = 0;
        {
            fstree_lock.WriteLock(key);
            ret = metadb->put(key.ToByteRange(), value.ToByteRange());
            fstree_lock.Unlock(key);
        }

        FreeInodeValue(value);

        if (ret == 0) {
            return 0;
        }
        else {
            errno = ENOENT;
            return FSError("MakeDir failed\n");
        }
    }

    int KVFS::OpenDir(StringPiece path, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("OpenDir: %s\n", path);
#endif
        _meta_key_t key{};
        StringPiece inode;
        if (!PathLookup(path, key)) {
            return FSError("OpenDir: No such file or directory\n");
        }
        fstree_lock.ReadLock(key);
        InodeCacheHandle *handle =
                                 inode_cache->Get(key, INODE_READ);
        if (handle != nullptr) {
            fi->fh = (uint64_t) handle;
            fstree_lock.Unlock(key);
            return 0;
        }
        else {
            fstree_lock.Unlock(key);
            return FSError("OpenDir: No such file or directory\n");
        }
    }

    int KVFS::ReadDir(StringPiece path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("ReadDir: %s\n", path);
#endif
        auto         *phandle      = (InodeCacheHandle *) fi->fh;
        _meta_key_t  childkey{};
        int          ret           = 0;
        kvfs_inode_t child_inumber = GetAttribute_str(phandle->value_)->st_ino;
        BuildMetaKey(child_inumber, (child_inumber == ROOT_INODE_ID) ? 1 : 0, childkey);
        RocksDBIterator *iter = metadb->GetNewIterator();
        if (filler(buf, ".", nullptr, 0) < 0) {
            return FSError("Cannot read a directory");
        }
        if (filler(buf, "..", nullptr, 0) < 0) {
            return FSError("Cannot read a directory");
        }
        for (iter->iter_->Seek(childkey.ToSlice());
             iter->iter_->Valid() && IsKeyInDir(iter->iter_->key(), childkey);
             iter->iter_->Next()) {
            StringPiece name_buffer = iter->iter_->value().data() + KVFS_INODE_HEADER_SIZE;
            if (name_buffer[0] == '\0') {
                continue;
            }
            if (filler(buf, name_buffer.data(), nullptr, 0) < 0) {
                ret = -1;
            }
            if (ret < 0) {
                break;
            }
        }
        delete iter;
        return ret;
    }

    int KVFS::ReleaseDir(StringPiece path, struct fuse_file_info *fi) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("ReleaseDir: %s\n", path);
#endif
        auto        *handle = (InodeCacheHandle *) fi->fh;
        _meta_key_t key     = handle->key_;
        inode_cache->Release(handle);
        return 0;
    }

    int KVFS::RemoveDir(StringPiece path) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("RemoveDir: %s\n", path);
#endif

        _meta_key_t key{};
        StringPiece inode;
        if (!PathLookup(path, key)) {
            return FSError("RemoveDir: No such file or directory\n");
        }
        int ret = 0;
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_DELETE);
        if (handle != nullptr) {
            dentry_cache->Evict(key);
            inode_cache->Release(handle);
            inode_cache->Evict(key);
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);
        return ret;
    }

    int KVFS::Rename(StringPiece old_path, StringPiece new_path) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Rename: %s %s\n", old_path, new_path);
#endif
        _meta_key_t old_key{};
        if (!PathLookup(old_path, old_key)) {
            return FSError("No such file or directory\n");
        }
        _meta_key_t new_key{};
        StringPiece filename;
        if (!PathLookup(new_path, new_key, filename)) {
            return FSError("No such file or directory\n");
        }

#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Rename old_key: [%08x, %08x]\n", old_key.inode_id, old_key.hash_id);
  state_->GetLog()->LogMsg("Rename new_key: [%08x, %08x]\n", new_key.inode_id, new_key.hash_id);
#endif

        _meta_key_t large_key{};
        _meta_key_t small_key{};
        if (old_key.inode_id > new_key.inode_id ||
            (old_key.inode_id == new_key.inode_id) &&
            (old_key.hash_id > new_key.hash_id)) {
            large_key = old_key;
            small_key = new_key;
        }
        else {
            large_key = new_key;
            small_key = old_key;
        }

        fstree_lock.WriteLock(large_key);
        fstree_lock.WriteLock(small_key);

        int              ret         = 0;
        InodeCacheHandle *old_handle = inode_cache->Get(old_key, INODE_DELETE);
        if (old_handle != nullptr) {
            const kvfs_inode_header *old_iheader = GetInodeHeader(old_handle->value_);

            StringPiece      new_value   = InitInodeValue(old_handle->value_, filename);
            InodeCacheHandle *new_handle = inode_cache->Insert(new_key, new_value);

            inode_cache->BatchCommit(old_handle, new_handle);
            inode_cache->Release(new_handle);
            inode_cache->Release(old_handle);
            inode_cache->Evict(old_key);
            dentry_cache->Evict(old_key);
            dentry_cache->Evict(new_key);
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(small_key);
        fstree_lock.Unlock(large_key);
        return ret;
    }

    int KVFS::Access(StringPiece path, int mask) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("Access: %s %08x\n", path, mask);
#endif
        //TODO: Implement Access
        return 0;
    }

    int KVFS::UpdateTimens(StringPiece path, const struct timespec tv[2]) {
#ifdef  KVFS_DEBUG
        state_->GetLog()->LogMsg("UpdateTimens: %s\n", path);
#endif
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("No such file or directory\n");
        }
        int ret = 0;
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_WRITE);
        if (handle != nullptr) {
            {
                const kvfs_stat_t *value    = GetAttribute_str(handle->value_);
                kvfs_stat_t       new_value = *value;
                new_value.st_atim.tv_sec  = tv[0].tv_sec;
                new_value.st_atim.tv_nsec = tv[0].tv_nsec;
                new_value.st_mtim.tv_sec  = tv[1].tv_sec;
                new_value.st_mtim.tv_nsec = tv[1].tv_nsec;
                UpdateAttribute(handle->value_, new_value);
                inode_cache->WriteBack(handle);
                inode_cache->Release(handle);
            }
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);
        return ret;
    }

    int KVFS::Chmod(StringPiece path, mode_t mode) {
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("No such file or directory\n");
        }
        int ret = 0;
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_WRITE);
        if (handle != nullptr) {
            const kvfs_stat_t *value    = GetAttribute_str(handle->value_);
            kvfs_stat_t       new_value = *value;
            new_value.st_mode = mode;
            UpdateAttribute(handle->value_, new_value);
            inode_cache->WriteBack(handle);
            inode_cache->Release(handle);
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);
        return ret;
    }

    int KVFS::Chown(StringPiece path, uid_t uid, gid_t gid) {
        _meta_key_t key{};
        if (!PathLookup(path, key)) {
            return FSError("No such file or directory\n");
        }
        int ret = 0;
        fstree_lock.WriteLock(key);
        InodeCacheHandle *handle = inode_cache->Get(key, INODE_WRITE);
        if (handle != nullptr) {
            const kvfs_stat_t *value    = GetAttribute_str(handle->value_);
            kvfs_stat_t       new_value = *value;
            new_value.st_uid = uid;
            new_value.st_gid = gid;
            UpdateAttribute(handle->value_, new_value);
            inode_cache->WriteBack(handle);
            inode_cache->Release(handle);
            return 0;
        }
        else {
            ret = -ENOENT;
        }
        fstree_lock.Unlock(key);
        return ret;
    }

    bool KVFS::GetStat(std::string stat, std::string *value) {
        return state_->GetMetaDB()->GetStat(stat, value);
    }

    void KVFS::Compact() {
        state_->GetMetaDB()->Compact();
    }

}
