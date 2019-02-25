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

//  InodeCacheEntry handle;
  InodeAccessMode access_mode;
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
  kvfsDirKey key = {current_key_.inode_, std::filesystem::hash_value(resolved_path_.filename())};
  const string &key_str = key.to_string();
  if (flags & O_CREAT) {
    // If set, the file will be created if it doesn’t already exist.
    if (flags & O_EXCL) {
      // If both O_CREAT and O_EXCL are set, then open fails if the specified file already exists.
      // This is guaranteed to never clobber an existing file.
      {
        mutex_->lock();
        if (store_->hasKey(key_str)) {
          // file exists return error
          mutex_->unlock();
          errorno_ = -EEXIST;
          throw FSError(FSErrorType::FS_EEXIST, "Flags O_CREAT and O_EXCL are set but the file already exists");
        }
      }
    }
    {
      // create a new file
      {
        // check if there is room for more open files
        if (next_free_fd_ == KVFS_MAX_OPEN_FILES) {
          errorno_ = -ENOSPC;
          throw FSError(FSErrorType::FS_ENOSPC, "Failed to open due to no more available file descriptors");
        }
      }
      // file doesn't exist so create new one
      // dirent.name
      auto name = resolved_path_.filename().string();
      auto md_ = kvfsMetaData(name, FreeInode(), mode, current_key_);
      // Acquire lock
      mutex_->lock();
      // generate a file descriptor
      auto fh_ = kvfsFileHandle(key, md_, flags);
      auto fd_ = next_free_fd_;
      ++next_free_fd_;

      // insert into store
      store_->put(key_str, md_.to_string());
      // write it back if flag O_SYNC
      if (flags & O_SYNC) {
        store_->sync();
      }
      // add it to open_fds
      open_fds_->at(fd_) = fh_;

      // release lock
      mutex_->unlock();
      return fd_;
    }
  }

  // flag is not O_CREAT so open existing file
  // Acquire lock
  mutex_->lock();

  auto sr = store_->get(key_str);
  // ensure sr is valid
  if (!sr.isValid()) {
    errorno_ = -EIO;
    return errorno_;
  }
  auto md_ = kvfsMetaData();
  md_.parse(sr);
  auto fh_ = kvfsFileHandle(key, md_, flags);

  auto fd_ = next_free_fd_;
  ++next_free_fd_;

  // add it to open_fds
  open_fds_->at(fd_) = fh_;

  mutex_->unlock();

  // not supported
  /*if (flags & O_TRUNC) {
    // check if file exists and then truncate its blocks to 0
    mutex_->lock();
    if (store_->hasKey(key_str)){
      auto sr = store_->get(key_str);
      kvfsMetaData md_ = kvfsMetaData();
      md_.parse(sr);
      // free the blocks of this file
      mutex_->unlock();
      if ( md_.fstat_.st_blocks > 1){
        bool status = FreeUpBlock(md_.block_key_);
        if (status){
          // success
        }
      }
      super_block_.total_block_count_ -= md_.fstat_.st_blocks;
      md_.fstat_.st_blocks = 0;

    }
  }*/

  return fd_;
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
bool kvfs::KVFS::FreeUpBlock(const kvfsBlockKey &key) {
  // check if freeblock key exists in store, loop through freeblocks
  // until we find the last freeblock key, store this key in there.
  // check free blocks count
  // divide by 512 if bigger than 512
  // generate key for freeblocks
  mutex_->lock();
  if (super_block_.freeblocks_count_ < 512) {
    // add it to first freeblocks block
    FreeBlocksKey fb_key = {"fb", 0};
    if (store_->hasKey(fb_key.to_string())) {
      auto sr = store_->get(fb_key.to_string());
      if (sr.isValid()) {
        FreeBlocksValue value;
        value.parse(sr);
        value.blocks[value.count_] = key;
        ++value.count_;
        // merge it in store
        auto status = store_->merge(fb_key.to_string(), value.to_string());
        if (!status) {
          throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
        }
      }
    } else {
      // generate the first fb block
      FreeBlocksValue value{};
      value.count_ = 1;
      value.blocks[0] = key;
    }
  } else {
    // fb count is >= 512
    uint64_t number = super_block_.freeblocks_count_ / 512;
    FreeBlocksKey fb_key = {"fb", number};
    FreeBlocksValue fb_value{};
    fb_value.blocks[fb_value.count_] = key;
    ++fb_value.count_;
    auto status = store_->merge(fb_key.to_string(), fb_value.to_string());
    if (!status) {
      throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
    }
    if (fb_value.count_ == 512) {
      // generate next fb_block
      ++fb_key.number_;
      FreeBlocksValue fb_value{};
      status = store_->put(fb_key.to_string(), fb_value.to_string());
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
      }
    }
  }
  mutex_->unlock();
  return true;
}
ssize_t kvfs::KVFS::Read(int filedes, void *buffer, size_t size) {
  try {
    // filedes must exist in open_fds
    auto fh_ = open_fds_->at(filedes);
    // determine how many to read
    std::memcpy(buffer, fh_.md_.inline_blck.data, fh_.md_.inline_blck.size_);
    size_t count = 1;
    if (size > KVFS_DEF_BLOCK_SIZE_4K) {
      auto to_read = size / KVFS_DEF_BLOCK_SIZE_4K;
      // copy inline
      // keep orig index of buffer
      auto org_idx = buffer;
      buffer = (static_cast<char *>(buffer)) + fh_.md_.inline_blck.size_;
      mutex_->lock();
      auto bkey = fh_.md_.block_key_.to_string();
      kvfsBlockValue bvalue;
      for (size_t i = 0; i < to_read; ++i) {
        // we assume buffer is byte aligned
        if (store_->hasKey(bkey)) {
          auto sr = store_->get(bkey);
          bvalue.parse(sr);
          std::memcpy(buffer, bvalue.data, bvalue.size_);
          bkey = bvalue.next_block_.to_string();
          buffer = (static_cast<char *>(buffer)) + bvalue.size_;
          count += 1;
        }
      }
      mutex_->unlock();
      buffer = org_idx;
    }
    return count;
  }
  catch (...) {
    std::cerr << "Failed to read. \n";
    return -1;
  }
}
ssize_t kvfs::KVFS::Write(int filedes, const void *buffer, size_t size) {
  auto fh_ = open_fds_->at(filedes);
  // check if inline data is not zero
  if (fh_.md_.inline_blck.size_ == 0) {
    // write inline first
    if (size < KVFS_DEF_BLOCK_SIZE_4K) {
      // fits into inline
      mutex_->lock();
      std::memcpy(fh_.md_.inline_blck.data, buffer, size);

      // update the stat
      fh_.md_.fstat_.st_blocks = 1;
      fh_.md_.fstat_.st_size = size;
      fh_.md_.fstat_.st_mtim.tv_sec = std::time(nullptr);
      // merge it in store
      auto status = store_->merge(fh_.key_.to_string(), fh_.md_.to_string());
      if (!status) {
        errorno_ = -EIO;
        return errorno_;
      }
      return size;
    } else {
      // size is bigger than 4K

    }
  }
  return 0;
}
