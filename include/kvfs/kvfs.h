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
#include "fs.h"

namespace kvfs {
class KVFS : public FS {
 public:
  KVFS(std::string mount_path);
  KVFS();
  ~KVFS();

 protected:
  char *GetCWD(char *buffer, size_t size) override;
  char *GetCurrentDirName() override;
  int ChDir(const char *filename) override;
  DIR *OpenDir(const char *dirname) override;
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

 private:
  std::shared_ptr<Store> store_;
  std::unique_ptr<InodeCache> inode_cache_;
  std::unique_ptr<DentryCache> dentry_cache_;
};
}  // namespace kvfs
#endif //KVFS_FILESYSTEM_H
