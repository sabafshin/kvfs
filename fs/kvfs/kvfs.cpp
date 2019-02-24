/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.cpp
 */

#include <kvfs/kvfs.h>

kvfs::KVFS::KVFS(const std::string &mount_path)
    : root_path(mount_path),
      store_(std::make_shared<RocksDBStore>(mount_path)),
      inode_cache_(std::make_unique<InodeCache>(KVFS_MAX_OPEN_FILES, store_)),
      dentry_cache_(std::make_unique<DentryCache>()),
      cwd_name_(root_path.parent_path().filename()),
      pwd_(root_path),
      open_fds_(std::make_unique<std::unordered_map<int, kvfs::kvfsFileHandle>>()),
      mutex_(std::make_unique<std::mutex>()) {
  FSInit();
}
kvfs::KVFS::KVFS()
    : root_path("/tmp/db/"),
      store_(std::make_shared<RocksDBStore>("/tmp/db/")),
      inode_cache_(std::make_unique<InodeCache>(store_)),
      dentry_cache_(std::make_unique<DentryCache>()),
      cwd_name_("db"),
      pwd_("/"),
      mutex_(std::make_unique<std::mutex>()) {
  FSInit();
}

kvfs::KVFS::~KVFS() {
  mutex_.reset();
  store_.reset();
  inode_cache_.reset();
  dentry_cache_.reset();
}
char *kvfs::KVFS::GetCWD(char *buffer, size_t size) {
  std::string result;
  if (size == 0 && buffer != nullptr) {
    errorno_ = -EINVAL;
    std::string msg = "The size argument is zero and buffer is not a null pointer.";
    throw FSError(FSErrorType::FS_EINVAL, msg);
  };

  if (cwd_name_.string().size() > size) {
    errorno_ = -ERANGE;
    std::string msg = "The size argument is less than the length of the working directory name.\n"
                      "You need to allocate a bigger array and try again.";
    throw FSError(FSErrorType::FS_ERANGE, msg);
  }

  // TODO: EACCESS not implemented!

  if (buffer != nullptr) {
    memcpy(buffer, cwd_name_.string().data(), cwd_name_.string().size());
    return nullptr;
  }

  return cwd_name_.string().data();
}
char *kvfs::KVFS::GetCurrentDirName() {
  std::string val_from_pwd = pwd_.filename();
  if (val_from_pwd != cwd_name_) {
    return val_from_pwd.data();
  }
  return cwd_name_.string().data();
}
int kvfs::KVFS::ChDir(const char *filename) {

  return 0;
}
DIR *kvfs::KVFS::OpenDir(const char *path) {
  kvfsDirKey key;
  if (!CheckNameLength(path)) {
    throw FSError(FSErrorType::FS_EINVAL, "Given path name does not comply with POSIX standards");
  }
  if (!Lookup(path, &key)) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "No such file or directory");
  }

  DIR *result;

  InodeCacheEntry cache_entry_;
  bool status = inode_cache_->get(key, INODE_READ, cache_entry_);
  if (status) {

  }

  return nullptr;
}
int kvfs::KVFS::Open(const char *filename, int flags, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    orig_ /= pwd_;
  }
  if (orig_.is_absolute()) {
    // must start with single "/"
    if (orig_.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }

  // Attemp to resolve the real path from given path
  std::filesystem::path resolved_path_ = ResolvePath(orig_);

  // now the path components all exist and resolved
  // check for O_CREAT flag

  InodeCacheEntry handle;
  InodeAccessMode access_mode;
  bool status = false;
  if ((flags & O_ACCMODE) == O_RDONLY) {
    access_mode = INODE_READ;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
    access_mode = INODE_WRITE;
  } else if ((flags & O_ACCMODE) == O_RDWR) {
    access_mode = INODE_RW;
  } else {
    // undefined access mode flags
    return -EINVAL;
  }
  if (flags & O_CREAT) {
    // If set, the file will be created if it doesn’t already exist.
    kvfsDirKey key = {current_key_.inode_, std::filesystem::hash_value(resolved_path_.filename())};
    if (flags & O_EXCL) {
      // If both O_CREAT and O_EXCL are set, then open fails if the specified file already exists.
      // This is guaranteed to never clobber an existing file.
      {
        mutex_->lock();
        if (inode_cache_->get(key, access_mode, handle)) {
          // file exists return error
          mutex_->unlock();
          errorno_ = -EEXIST;
          throw FSError(FSErrorType::FS_EEXIST, "Flags O_CREAT and O_EXCL are set but the file already exists");
        }
      }
      {
        {
          // check if there is room for more open files
          if (next_free_fd_ == KVFS_MAX_OPEN_FILES) {
            errorno_ = ENOMEM;
            throw FSError(FSErrorType::FS_ENOSPC, "Failed to open due to no more available file descriptors");
          }
        }
        // file doesn't exist so create new one
        kvfsMetaData md_{};
        // dirent.name
        resolved_path_.string().copy(md_.dirent_.d_name, resolved_path_.string().length());
        // get a free inode for this new file
        md_.dirent_.d_ino = FreeInode();
        md_.fstat_.st_ino = md_.dirent_.d_ino;
        // generate stat
        md_.fstat_.st_mode = mode;
        md_.fstat_.st_blocks = 0;
        md_.fstat_.st_ctim.tv_sec = std::time(nullptr);
        md_.fstat_.st_blksize = KVFS_DEF_BLOCK_SIZE_4K;
        // owner
        md_.fstat_.st_uid = getuid();
        md_.fstat_.st_gid = getgid();
        // link count
        if (S_ISREG(mode)) {
          md_.fstat_.st_nlink = 1;
        } else {
          md_.fstat_.st_nlink = 2;
        }
        md_.fstat_.st_blocks = 0;
        md_.fstat_.st_size = 0;
        // set parent
        md_.parent_key_ = current_key_;

        // Acquire lock
        mutex_->lock();
        // generate a file descriptor
        kvfsFileHandle fh_{};
        int fd_ = next_free_fd_;
        ++next_free_fd_;

      }
    }
  }

  if (flags & O_TRUNC) {
  }

  if (flags & O_APPEND) {
  }

  int ret = 0;
  if (status) {

  }
  return 0;
}
bool kvfs::KVFS::Lookup(const char *buffer, kvfs::kvfsDirKey *key) {
  const char *lpos;
  kvfs_file_inode_t inode_in_search;
  if (ParentLookup(buffer, key, inode_in_search, lpos)) {
    const char *rpos = strchr(lpos, '\0');
    if (rpos != nullptr && rpos - lpos > 1) {
      BuildKey(std::string(lpos + 1), static_cast<const int>(rpos - lpos - 1), inode_in_search, key);
    }
    return true;
  } else {
    errorno_ = -ENOENT;
    return false;
  }
}
void kvfs::KVFS::FSInit() {
  if (store_ != nullptr) {
    mutex_->lock();
    if (store_->hasKey("superblock")) {
      auto sb = store_->get("superblock");
      super_block_.parse(sb);
      super_block_.fs_number_of_mounts_++;
      super_block_.fs_last_mount_time_ = time_now;

    } else {
      super_block_.fs_creation_time_ = time_now;
      super_block_.fs_last_mount_time_ = time_now;
      super_block_.fs_number_of_mounts_ = 1;
      super_block_.total_block_count_ = 0;
      super_block_.total_inode_count_ = 1;
      super_block_.next_free_inode_ = 0;
    }
    mutex_->unlock();
  } else {
    throw FSError(FSErrorType::FS_EIO, "Failed to initialise the file system");
  }
}
bool kvfs::KVFS::CheckNameLength(const std::filesystem::path &path) {
  std::string path_string = path.string();
  if (path.empty()) {
    throw FSError(FSErrorType::FS_EINVAL, "Given path name is empty!");
  }
  if (path.filename().string().length() > NAME_MAX) {
    errorno_ = -ENAMETOOLONG;
    throw FSError(FSErrorType::FS_ENAMETOOLONG, "Given file name is longer than NAME_MAX");
  }
  return true;
}
void kvfs::KVFS::BuildKey(std::basic_string<char> basic_string,
                          const int i,
                          kvfs::kvfs_file_inode_t search,
                          kvfs::kvfsDirKey *key) {

}

bool kvfs::KVFS::ParentLookup(const char *buffer,
                              kvfs::kvfsDirKey *key,
                              kvfs::kvfs_file_inode_t search,
                              const char *lastdelimiter) {
  const char *lpos = buffer;
  const char *rpos;
  bool flag_found = true;
  std::string item;
  search = 0;
  kvfsMetaData in_search;
  while ((rpos = strchr(lpos + 1, KVFS_PATH_DELIMITER)) != nullptr) {
    if (rpos - lpos > 0) {
      BuildKey(std::string(lpos + 1), static_cast<const int>(rpos - lpos - 1), search, key);
      if (!dentry_cache_->find(*key, in_search)) {
        {
          StoreResult result = store_->get(key->to_string());
          if (result.isValid()) {
            in_search.parse(result);
            search = in_search.fstat_.st_ino;
            dentry_cache_->insert(*key, in_search);
          } else {
            errorno_ = -ENOENT;
            flag_found = false;
          }
          if (!flag_found) {
            return false;
          }
        }
      }
    }
    lpos = rpos;
  }
  if (lpos == buffer) {
    BuildKey("", 0, 0, key);
  }
  lastdelimiter = lpos;
  return flag_found;
}
std::filesystem::path kvfs::KVFS::ResolvePath(const std::filesystem::path &input) {
  std::filesystem::path output;
  kvfs_file_inode_t inode = 0;
  // Assume the path is absolute
  for (const std::filesystem::path &e : input) {
    if (e == ".") {
      continue;
    }
    if (e == "..") {
      output = output.parent_path();
      continue;
    }

    // check if name is not too long
    if (!CheckNameLength(e)) {
      errorno_ = -ENAMETOOLONG;
      throw FSError(FSErrorType::FS_ENAMETOOLONG, "File name is longer than POSIX NAME_MAX");
    }

    // Request the metadata from store
    {
      mutex_->lock();
      kvfsDirKey key = {inode, std::filesystem::hash_value(e)};
      InodeCacheEntry handle;
      if (inode_cache_->get(key, INODE_READ, handle)) {
        // found the key in store
        bool is_link = S_ISLNK(handle.md_.fstat_.st_mode);
        if (is_link) {
          // we occurred a symbolic link, prefix the path with the contents
          // of this link
          mutex_->unlock();
          output = GetSymLinkRealPath(handle.md_);
          current_key_ = handle.md_.real_key_;
        } else {
          current_key_ = handle.key_;
        }
        // update inode to this file's inode
        inode = handle.md_.fstat_.st_ino;
      } else {
        // the path component must exists
        // otherwise we are at the end and we just append the filename
        // file name will be checked in the calling function
        if (e == input.filename()) {
          // we at the end here
          output.append(e.string());
        } else {
          errorno_ = -ENONET;
          throw FSError(FSErrorType::FS_ENOENT, "No such file or directory found.");
        }
      }
    }
    // append normally
    output.append(e.string());
  }
  return output;
}
std::filesystem::path kvfs::KVFS::GetSymLinkRealPath(const kvfs::kvfsMetaData &data) {
  std::filesystem::path output = "/";
  std::list<std::filesystem::path> path_list;
  path_list.push_front(data.dirent_.d_name);
  // lock for cache access
  mutex_->lock();
  auto key = data.parent_key_;
  InodeCacheEntry handle;
  errorno_ = -0;
  while (key.inode_ > 0) {
    if (inode_cache_->get(key, INODE_READ, handle)) {
      key = handle.md_.parent_key_;
      path_list.push_front(handle.md_.dirent_.d_name);
    } else {
      errorno_ = -ENONET;
      throw FSError(FSErrorType::FS_ENOENT, "No such file or directory found.");
    }
  }
  for (const auto &p : path_list) {
    output.append(p.string());
  }
  mutex_->unlock();
  return output;
}
bool kvfs::KVFS::starts_with(const std::string &s1, const std::string &s2) {
  return s2.size() <= s1.size() && s1.compare(0, s2.size(), s2) == 0;
}
kvfs::kvfs_file_inode_t kvfs::KVFS::FreeInode() {
  kvfs_file_inode_t fi = super_block_.next_free_inode_;
  ++super_block_.next_free_inode_;
  ++super_block_.total_inode_count_;
  return fi;
}
