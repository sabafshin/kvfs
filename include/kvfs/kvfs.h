/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.h
 */

#ifndef KVFS_FILESYSTEM_H
#define KVFS_FILESYSTEM_H

#include <kvfs_rocksdb/kvfs_rocksdb_store.h>
#include <inodes/directory_entry_cache.h>
#include <inodes/inode_cache.h>
#include <kvfs/super.h>
#include <kvfs_utils/mutex.h>
#include <time.h>
#include <fcntl.h>
#include <kvfs/fs_error.h>
#include "fs.h"
#include <filesystem>
#include <limits>
#include <stdlib.h>
#include <mutex>
#include <iostream>
#include <array>
#include <algorithm>
#include <string.h>
#include <cstring>
#include <linux/fs.h>
#include <unistd.h>

namespace kvfs {

#define time_now std::time(nullptr)

class KVFS : public FS {
 public:
  explicit KVFS(const std::string &mount_path);
  KVFS();
  ~KVFS();

 protected:
  char *GetCWD(char *buffer, size_t size) override;
  char *GetCurrentDirName() override;
  int ChDir(const char *path) override;
  kvfsDIR *OpenDir(const char *path) override;
  kvfs_dirent *ReadDir(kvfsDIR *dirstream) override;
  int CloseDir(kvfsDIR *dirstream) override;
  int Link(const char *oldname, const char *newname) override;
  int SymLink(const char *path1, const char *path2) override;
  ssize_t ReadLink(const char *filename, char *buffer, size_t size) override;
  int UnLink(const char *filename) override;
  int RmDir(const char *filename) override;
  int Remove(const char *filename) override;
  int Rename(const char *oldname, const char *newname) override;
  int MkDir(const char *filename, mode_t mode) override;
  int Stat(const char *filename, kvfs_stat *buf) override;
  int ChMod(const char *filename, mode_t mode) override;
  int Access(const char *filename, int how) override;
  int UTime(const char *filename, const struct utimbuf *times) override;
  int Truncate(const char *filename, off_t length) override;
  int Mknod(const char *filename, mode_t mode, dev_t dev) override;
  void TuneFS() override;
  int Open(const char *filename, int flags, mode_t mode) override;
  int Close(int filedes) override;
  ssize_t Read(int filedes, void *buffer, size_t size) override;
  ssize_t Write(int filedes, const void *buffer, size_t size) override;
  off_t LSeek(int filedes, off_t offset, int whence) override;
  ssize_t CopyFileRange(int inputfd,
                        off64_t *inputpos,
                        int outputfd,
                        off64_t *outputpos,
                        ssize_t length,
                        unsigned int flags) override;
  void Sync() override;
  int FSync(int filedes) override;
  ssize_t PRead(int filedes, void *buffer, size_t size, off_t offset) override;
  ssize_t PWrite(int filedes, const void *buffer, size_t size, off_t offset) override;
  void DestroyFS() override;
 private:
  std::filesystem::path root_path;
  std::shared_ptr<Store> store_;
  std::unique_ptr<InodeCache> inode_cache_;
  std::unique_ptr<DentryCache> open_fds_;
  kvfsSuperBlock super_block_{};
  int8_t errorno_;
  std::filesystem::path cwd_name_;
  std::filesystem::path pwd_;
  kvfsMetaData current_md_{};
  kvfsDirKey current_key_{};
  uint32_t next_free_fd_{};
  std::unique_ptr<std::mutex> mutex_;
  // Private Methods
 private:
  void FSInit();
  bool CheckNameLength(const std::filesystem::path &path);
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> ResolvePath(const std::filesystem::path &input);
  inline bool starts_with(const std::string &s1, const std::string &s2);
  std::filesystem::path GetSymLinkContentsPath(const kvfsMetaData &data);
  kvfs_file_inode_t GetFreeInode();
  bool FreeUpBlock(const kvfsBlockKey &key);
  kvfsBlockKey GetFreeBlock();
  uint32_t GetFreeFD();
  std::pair<ssize_t, kvfs::kvfsBlockKey> WriteBlocks(kvfsBlockKey blck_key_,
                                                     size_t blcks_to_write_,
                                                     const void *buffer,
                                                     size_t buffer_size_,
                                                     kvfs_off_t offset);
  std::pair<ssize_t, const void *> FillBlock(kvfsBlockValue *blck_,
                                             const void *buffer,
                                             size_t buffer_size_,
                                             kvfs_off_t offset);
  std::pair<ssize_t, void *> ReadBlock(kvfsBlockValue *blck_, void *buffer, size_t size, off_t offset);
  ssize_t ReadBlocks(kvfsBlockKey blck_key_,
                     size_t blcks_to_read_,
                     size_t buffer_size_,
                     off_t offset,
                     void *buffer);
  std::pair<std::filesystem::path,
            std::pair<kvfs::kvfsDirKey, kvfs::kvfsMetaData>> RealPath(const std::filesystem::path &input);
};

}  // namespace kvfs

struct __kvfs_dir_stream {
  uint32_t file_descriptor_;
  std::unique_ptr<kvfs::Store::Iterator> ptr_;
};

#endif //KVFS_FILESYSTEM_H