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

#include <kvfs_rocksdb/rocksdb_store.h>
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

namespace kvfs {

#define time_now std::time(nullptr)

struct kvfsFileHandle {
  kvfsDirKey key_;
  kvfsMetaData md_;
};

class KVFS : public FS {
 public:
  explicit KVFS(const std::string &mount_path);
  KVFS();
  ~KVFS();

 protected:
  char *GetCWD(char *buffer, size_t size) override;
  char *GetCurrentDirName() override;
  int ChDir(const char *filename) override;
  DIR *OpenDir(const char *path) override;
  struct dirent *ReadDir(DIR *dirstream) override;
  struct dirent64 *ReadDir64(DIR *dirstream) override;
  int CloseDir(DIR *dirstream) override;
  int Link(const char *oldname, const char *newname) override;
  int SymLink(const char *oldname, const char *newname) override;
  ssize_t ReadLink(const char *filename, char *buffer, size_t size) override;
  int UnLink(const char *filename) override;
  int RmDir(const char *filename) override;
  int Remove(const char *filename) override;
  int Rename(const char *oldname, const char *newname) override;
  int MkDir(const char *filename, mode_t mode) override;
  int Stat(const char *filename, struct stat *buf) override;
  int Stat64(const char *filename, struct stat64 *buf) override;
  int ChMod(const char *filename, mode_t mode) override;
  int Access(const char *filename, int how) override;
  int UTime(const char *filename, const struct utimbuf *times) override;
  int Truncate(const char *filename, off_t length) override;
  int Truncate64(const char *name, off64_t length) override;
  int Mknod(const char *filename, mode_t mode, dev_t dev) override;
  void TuneFS();
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
  int FSync(int fildes) override;


 private:
  std::filesystem::path root_path;
  std::shared_ptr<Store> store_;
  std::unique_ptr<InodeCache> inode_cache_;
  std::unique_ptr<DentryCache> dentry_cache_;
  SuperBlock super_block_;
  int8_t errorno_;
  std::filesystem::path cwd_name_;
  std::filesystem::path pwd_;
  kvfs_stat current_stat_;
  kvfsDirKey current_key_;
  int next_free_fd_;
  std::unique_ptr<std::unordered_map<int, kvfs::kvfsFileHandle>> open_fds_;
  std::unique_ptr<std::mutex> mutex_;
  // Private Methods
 private:
  void FSInit();
  inline bool Lookup(const char *filename, kvfsDirKey *key);
  bool CheckNameLength(const std::filesystem::path &path);
  inline void BuildKey(std::basic_string<char> basic_string, int i, kvfs_file_inode_t search, kvfsDirKey *key);
  inline bool ParentLookup(const char *buffer, kvfsDirKey *key, kvfs_file_inode_t search, const char *lpos);
  std::filesystem::path ResolvePath(const std::filesystem::path &input);
  inline bool starts_with(const std::string &s1, const std::string &s2);
  std::filesystem::path GetSymLinkRealPath(const kvfsMetaData &data);
  kvfs_file_inode_t FreeInode();
};
}  // namespace kvfs
#endif //KVFS_FILESYSTEM_H