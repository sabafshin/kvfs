/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.cpp
 */

#include <kvfs/kvfs.h>

namespace kvfs {
kvfs::KVFS::KVFS(const std::string &mount_path)
    : root_path(mount_path),
      store_(std::make_shared<RocksDBStore>(mount_path)),
      inode_cache_(std::make_unique<InodeCache>(KVFS_MAX_OPEN_FILES, store_)),
      open_fds_(std::make_unique<DentryCache>(512)),
      cwd_name_(""),
      pwd_("/"),
      errorno_(0),
      current_key_{},
      current_md_{},
      next_free_fd_(0),
      mutex_(std::make_unique<std::mutex>()) {
  FSInit();
}
kvfs::KVFS::KVFS()
    : root_path("/tmp/db/"),
      store_(std::make_shared<RocksDBStore>("/tmp/db/")),
      inode_cache_(std::make_unique<InodeCache>(512, store_)),
      open_fds_(std::make_unique<DentryCache>(512)),
      cwd_name_(""),
      pwd_("/"),
      errorno_(0),
      current_key_{},
      current_md_{},
      next_free_fd_(0),
      mutex_(std::make_unique<std::mutex>()) {
  FSInit();
}

kvfs::KVFS::~KVFS() {
  mutex_.reset();
  inode_cache_.reset();
  open_fds_.reset();
  store_.reset();
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

  return pwd_.string().data();
}
char *kvfs::KVFS::GetCurrentDirName() {
  std::string val_from_pwd = pwd_.filename();
  if (val_from_pwd != cwd_name_) {
    return val_from_pwd.data();
  }
  return pwd_.string().data();
}
int kvfs::KVFS::ChDir(const char *filename) {

  return 0;
}
kvfsDIR *KVFS::OpenDir(const char *path) {
  auto orig_ = std::filesystem::path(path);
  if (orig_.is_relative()) {
    // make it absolute
    auto prv_ = pwd_;
    orig_ = pwd_.append(orig_.string());
    pwd_ = prv_;
  }
  if (orig_.is_absolute()) {
    // must start with single "/"
    if (orig_.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }

  // Attemp to resolve the real path from given path
  auto resolved = ResolvePath(orig_);

  // lock
  mutex_->lock();
  kvfs_file_hash_t hash = 0;

  if (resolved.first != "/") {
    hash = std::filesystem::hash_value(resolved.first.filename());
  } else {
    hash = std::filesystem::hash_value(resolved.first);
  }

  kvfsDirKey key = {resolved.second.first.inode_, hash};
  const string &key_str = key.pack();
  if (!store_->hasKey(key_str)) {
    // does not exist return error
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "A component of dirname does not name an existing directory "
                                          "or dirname is an empty string.");
  }
  auto sr = store_->get(key_str);
  if (sr.isValid()) {
    kvfsMetaData md_;
    md_.parse(sr);

    // check if it is a directory
    if (!S_ISDIR(md_.fstat_.st_mode)) {
      errorno_ = -ENOTDIR;
      throw FSError(FSErrorType::FS_ENOTDIR, "A component of dirname names an existing file that "
                                             "is neither a directory nor a symbolic link to a directory.");
    }

    // insert it into open_fds
    auto fd_ = GetFreeFD();
    auto fh_ = kvfsFileHandle();
    fh_.md_ = md_;
    fh_.key_ = key;

    open_fds_->insert(fd_, fh_);

    // set current key to this one
//    current_key_ = key;
//    current_md_ = md_;

//    cwd_name_.append(md_.dirent_.d_name);
//    pwd_.append(resolved.first.string());

    //unlock
    mutex_->unlock();

    //success
    auto *result = new kvfsDIR();
    result->file_descriptor_ = fd_;
    result->offset_ = 0;
    return result;
  }

  mutex_->unlock();
  return nullptr;
}
int kvfs::KVFS::Open(const char *filename, int flags, mode_t mode) {
  auto orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    auto prv_ = pwd_;
    orig_ = pwd_.append(orig_.string());
    pwd_ = prv_;
  }
  if (orig_.is_absolute()) {
    // must start with single "/"
    if (orig_.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }

  // Attemp to resolve the real path from given path
  auto resolved = ResolvePath(orig_);

//  std::cout << resolved_path_ << std::endl;

  // now the path components all exist and resolved
  // check for O_CREAT flag

//  InodeCacheEntry handle;
//  InodeAccessMode access_mode;
  if ((flags & O_ACCMODE) == O_RDONLY) {
//    access_mode = INODE_READ;
  } else if ((flags & O_ACCMODE) == O_WRONLY) {
//    access_mode = INODE_WRITE;
  } else if ((flags & O_ACCMODE) == O_RDWR) {
//    access_mode = INODE_RW;
  } else {
    // undefined access mode flags
    return -EINVAL;
  }

  // check name of file
  if (resolved.first.filename().empty()) {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "File name is empty!");
  }

  kvfs_file_hash_t hash = std::filesystem::hash_value(resolved.first);
  kvfsDirKey key = {resolved.second.first.inode_, hash};

  const string &key_str = key.pack();
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
      auto name = resolved.first.filename().string();
      auto md_ = kvfsMetaData(name, GetFreeInode(), mode, current_key_);
      // Acquire lock
      mutex_->lock();

      md_.dirent_.d_off = resolved.second.second.children_offset_;
      ++resolved.second.second.children_offset_;

      // generate a file descriptor
      auto fh_ = kvfsFileHandle(key, md_, flags);
      auto fd_ = next_free_fd_;
      ++next_free_fd_;

      // insert into store
      store_->put(key_str, md_.pack());
      store_->put(resolved.second.first.pack(), resolved.second.second.pack());
      // write it back if flag O_SYNC
      if (flags & O_SYNC) {
        store_->sync();
      }
      // add it to open_fds
      open_fds_->insert(fd_, fh_);

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
  open_fds_->insert(fd_, fh_);

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
        bool status = FreeUpBlock(md_.first_block_key_);
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
void kvfs::KVFS::FSInit() {
  if (store_ != nullptr) {
    mutex_->lock();
    if (store_->hasKey("superblock")) {
      auto sb = store_->get("superblock");
      if (sb.isValid()) {
        super_block_.parse(sb);
        super_block_.fs_number_of_mounts_++;
        super_block_.fs_last_mount_time_ = time_now;
      } else {
        throw FSError(FSErrorType::FS_EIO, "Failed to IO with store");
      }
    } else {
      super_block_.fs_creation_time_ = time_now;
      super_block_.fs_last_mount_time_ = time_now;
      super_block_.fs_number_of_mounts_ = 1;
      super_block_.total_block_count_ = 0;
      super_block_.total_inode_count_ = 1;
      super_block_.next_free_inode_ = 1;
      auto status = store_->put("superblock", super_block_.pack());
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to IO with store");
      }
    }
    kvfsDirKey root_key = {0, std::filesystem::hash_value("/")};
    if (!store_->hasKey(root_key.pack())) {
      mode_t mode = geteuid() | getegid() | S_IFDIR;
      kvfsMetaData root_md = kvfsMetaData("/", 0, mode, root_key);
      store_->put(root_key.pack(), root_md.pack());
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

std::pair<std::filesystem::path,
          std::pair<kvfsDirKey, kvfsMetaData>> kvfs::KVFS::ResolvePath(const std::filesystem::path &input) {
//  std::cout << input << std::endl;
  std::filesystem::path output;
  kvfs_file_inode_t inode = 0;
  int invalid_count = 0;
  kvfsDirKey parent_key_;
  kvfsMetaData parent_md_;
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
      kvfsMetaData md_{};
      if (store_->hasKey(key.pack())) {
        // found the key in store
        auto sr = store_->get(key.pack());
        md_.parse(sr);
        bool is_link = S_ISLNK(md_.fstat_.st_mode);
        if (is_link) {
          // we occurred a symbolic link, prefix the path with the contents
          // of this link
          mutex_->unlock();
          output = GetSymLinkRealPath(md_);
          parent_key_ = md_.real_key_;
        } else {
          parent_key_ = key;
        }
        // update inode to this file's inode
        inode = md_.fstat_.st_ino;
        parent_md_ = md_;
      } else {
        // the path component must exists
        // otherwise we are at the end and we just append the filename
        // file name will be checked in the calling function
        /*if (e != input.filename()) {
          errorno_ = -ENONET;
          throw FSError(FSErrorType::FS_ENOENT, "No such file or directory found.");
        }*/
        ++invalid_count;
      }
      mutex_->unlock();
    }
    // append normally
    output.append(e.string());
  }

  if (invalid_count > 1) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "No such file or directory found.");
  }

  return std::pair(output, std::pair(parent_key_, parent_md_));
}
std::filesystem::path kvfs::KVFS::GetSymLinkRealPath(const kvfs::kvfsMetaData &data) {
  std::filesystem::path output = "/";
  std::list<std::filesystem::path> path_list;
  path_list.push_front(data.dirent_.d_name);
  // lock for cache access
  mutex_->lock();
  auto key = data.parent_key_;
  kvfsMetaData md_;
  errorno_ = -0;
  while (key.inode_ > 0) {
    if (store_->hasKey(key.pack())) {
      auto sr = store_->get(key.pack());
      md_.parse(sr);
      key = md_.parent_key_;
      path_list.push_front(md_.dirent_.d_name);
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
kvfs_file_inode_t kvfs::KVFS::GetFreeInode() {
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
    kvfsFreeBlocksKey fb_key = {"fb", 0};
    if (store_->hasKey(fb_key.pack())) {
      auto sr = store_->get(fb_key.pack());
      if (sr.isValid()) {
        kvfsFreeBlocksValue value{};
        value.parse(sr);
        value.blocks[value.count_] = key;
        ++value.count_;
        // merge it in store
        auto status = store_->merge(fb_key.pack(), value.pack());
        if (!status) {
          throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
        }
        if (value.count_ == 512) {
          // generate next fb_block
          ++fb_key.number_;
          kvfsFreeBlocksValue fb_value{};
          status = store_->put(fb_key.pack(), fb_value.pack());
          if (!status) {
            throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
          }
        }
      }
    } else {
      // generate the first fb block
      kvfsFreeBlocksValue value{};
      value.count_ = 1;
      ++super_block_.freeblocks_count_;
      value.blocks[0] = key;
      // put in store
      auto status = store_->put(fb_key.pack(), value.pack());
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
      }
      return status;
    }
  } else {
    // fb count is >= 512
    uint64_t number = super_block_.freeblocks_count_ / 512;
    kvfsFreeBlocksKey fb_key = {"fb", number};
    kvfsFreeBlocksValue fb_value{};
    fb_value.blocks[fb_value.count_] = key;
    ++fb_value.count_;
    ++super_block_.freeblocks_count_;
    auto status = store_->merge(fb_key.pack(), fb_value.pack());
    if (!status) {
      throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
    }
    if (fb_value.count_ == 512) {
      // generate next fb_block
      ++fb_key.number_;
      kvfsFreeBlocksValue fb_value{};
      status = store_->put(fb_key.pack(), fb_value.pack());
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to perform IO in the store");
      }
    }
  }
  mutex_->unlock();
  return true;
}
ssize_t kvfs::KVFS::Read(int filedes, void *buffer, size_t size) {
  // read from offset in the file descriptor
  // if offset is at eof then return 0 else try to pread with filedes offset
  kvfsFileHandle fh_;
  auto status = open_fds_->find(filedes, fh_);
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first

  if ((fh_.flags_ & O_NONBLOCK) > 0) {
    // return error without writing anything
    errorno_ = -ECANCELED;
    return errorno_;
  }

  if ((fh_.flags_ & O_RDONLY) == 0) {
    // file was not opened with read flag
    return 0;
  }

  if (fh_.offset_ == fh_.md_.fstat_.st_size) {
    // eof

    // update stats and return
    mutex_->lock();
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->insert(filedes, fh_);
    mutex_->unlock();
    return 0;
  } else {
    // try to read the file at the file descriptor position
    // then modify filedes offset by amount read
    return PRead(filedes, buffer, size, fh_.offset_);
  }
}
ssize_t kvfs::KVFS::Write(int filedes, const void *buffer, size_t size) {
  kvfsFileHandle fh_;
  auto status = open_fds_->find(filedes, fh_);
  ssize_t written = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  kvfsDirKey owner = (S_ISLNK(fh_.md_.fstat_.st_mode)) ? fh_.md_.real_key_ : fh_.key_;
  // check flags first

  if ((fh_.flags_ & O_NONBLOCK) > 0) {
    // return without writting anything
    errorno_ = -ECANCELED;
    return errorno_;
  }
  // check if O_APPEND is set, then always append
  bool append_only = (fh_.flags_ & O_APPEND) > 0;
  if (append_only) {
    // ignore offset and append only

    // determine which block to write from, then
    // pass that to blocks writer.

    if (fh_.md_.inline_blck.size_ < KVFS_DEF_BLOCK_SIZE) {
      // update inline data then write left over to blocks
      mutex_->lock();
      auto pair = FillBlock(&fh_.md_.inline_blck, buffer, size, owner);
      written = pair.first;
      mutex_->unlock();
      if (written == static_cast<ssize_t>(size)) {
        // finished, buffer fitted into inline block
        goto out;
      }

      kvfsBlockKey new_blck = GetFreeBlock();
      new_blck.offset_ += KVFS_DEF_BLOCK_SIZE;
      mutex_->lock();
      fh_.md_.inline_blck.next_block_ = new_blck;
      mutex_->unlock();

      size_t size_left = size - written;
      size_t blocks_to_write_ = size_left / KVFS_DEF_BLOCK_SIZE;
      blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;

      auto result =
          WriteBlocks(fh_.md_.inline_blck.next_block_, blocks_to_write_, pair.second, size - written, owner);
      written += result.first;
      mutex_->lock();
      fh_.md_.last_block_key_ = result.second;
      mutex_->unlock();
      // finished
      goto out;
    } else {
      mutex_->lock();
      auto updated_key = fh_.md_.last_block_key_;
      StoreResult sr = store_->get(fh_.md_.last_block_key_.pack());
      mutex_->unlock();
      if (sr.isValid()) {
        auto *bv_ = new kvfsBlockValue();
        bv_->parse(sr);
        // fill this block then write left over blocks
        auto pair = FillBlock(bv_, buffer, size, owner);
        written = pair.first;
        if (written == static_cast<ssize_t>(size)) {
          // finished, buffer fitted in last block
          goto out;
        }

        bv_->next_block_ = GetFreeBlock();
        bv_->next_block_.offset_ += fh_.md_.last_block_key_.offset_ + KVFS_DEF_BLOCK_SIZE;

        size_t size_left = size - written;
        size_t blocks_to_write_ = size_left / KVFS_DEF_BLOCK_SIZE;
        blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;

        auto result = WriteBlocks(bv_->next_block_, blocks_to_write_, pair.second, size - written, owner);
        written += result.first;

        mutex_->lock();
        fh_.md_.last_block_key_ = result.second;
        store_->put(updated_key.pack(), bv_->pack());
        mutex_->unlock();
        delete (bv_);
        // finished
        goto out;
      } else {
        // we shouldn't reach here ideally
        errorno_ = -EINTR;
        throw FSError(FSErrorType::FS_EINTR, "The function write exited prematurely");
      }
    }
    out:
    // update stats and return
    mutex_->lock();
    fh_.md_.fstat_.st_size += written;
    fh_.offset_ += written;
    fh_.md_.fstat_.st_mtim.tv_sec = time_now;
    open_fds_->insert(filedes, fh_);
    mutex_->unlock();
    return written;

  } else {
    // call to PWrite to write from offset
    return this->PWrite(filedes, buffer, size, fh_.offset_);
  }
}
kvfs::kvfsBlockKey kvfs::KVFS::GetFreeBlock() {
  // search free block first
  mutex_->lock();
  if (super_block_.freeblocks_count_ != 0) {
    auto fb_number = super_block_.freeblocks_count_ / 512;
    kvfsFreeBlocksKey fb_key = {"fb", fb_number};
    auto sr = store_->get(fb_key.pack());
    if (sr.isValid()) {
      kvfsFreeBlocksValue val{};
      val.parse(sr);
      auto key = val.blocks[val.count_ - 1];

      // check if the block refers to another block
      kvfsBlockValue bv_{};
      auto in_key_ = key;
      if (store_->hasKey(in_key_.pack())) {
        sr = store_->get(in_key_.pack());
        if (sr.isValid()) {
          bv_.parse(sr);
          if (bv_.next_block_.block_number_ != 0) {
            in_key_ = bv_.next_block_;
          }
        }
      }
      if (in_key_.block_number_ == key.block_number_) {
        // the block is solo
        // mark it zero
        val.blocks[val.count_ - 1] = kvfsBlockKey();
      } else {
        // the block referred to another block
        // delete it from array
        val.blocks[val.count_ - 1] = in_key_;
      }
      --val.count_;
      if (val.count_ == 0) {
        // its an empty array now, delete it from store
        store_->delete_(fb_key.pack());
      }
      --super_block_.freeblocks_count_;
      // unlock
      mutex_->unlock();
      return key;
    }
  }
  // get a free block from superblock
  kvfsBlockKey bk_ = {0, super_block_.next_free_block_number};
  ++super_block_.next_free_block_number;
  ++super_block_.total_block_count_;

  mutex_->unlock();
  return bk_;
}
int KVFS::Close(int filedes) {
  // check filedes exists
  try {
    kvfsFileHandle fh_;
    mutex_->lock();
    auto status = open_fds_->find(filedes, fh_);
    if (!status) {
      errorno_ = -EBADFD;
      return errorno_;
    }
    // update store
    // check if file exists in the store
    // if file is in store, check it's mtime
    // if it matches this file des then update it otherwise just evict this file des
    // if it doesn't exist then assume it is a new file
    if (store_->hasKey(fh_.key_.pack())) {
      auto sr = store_->get(fh_.key_.pack());
      if (sr.isValid()) {
        kvfsMetaData check_md_;
        check_md_.parse(sr);
        if (check_md_.fstat_.st_mtim.tv_sec > fh_.md_.fstat_.st_mtim.tv_sec) {
          // file was updated in another file des recently
          // evict only
          open_fds_->evict(filedes);
          mutex_->unlock();
          return 0;
        }
      }
    }
    // file is new or updated
    store_->merge(fh_.key_.pack(), fh_.md_.pack());
    store_->sync();
    // release it from open_fds
    open_fds_->evict(filedes);
    // success
    mutex_->unlock();
    return 0;
  } catch (...) {
    errorno_ = -EINTR;
    throw FSError(FSErrorType::FS_EINTR, "The close() function was interrupted by a signal.");
  }
}
kvfs_dirent *KVFS::ReadDir(kvfsDIR *dirstream) {
  // check if the fd is in open_fds
  kvfsFileHandle fh_;

  if (!dirstream) {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The dirp argument is a nullptr");
  }
  mutex_->lock();
  if (!open_fds_->find(dirstream->file_descriptor_, fh_)) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The dirp argument does not refer to an open directory stream.");
  }
  mutex_->unlock();
  try {
    // start from offset on dirstream
    // for each dirsteam get next one using get_next from prefix.
    mutex_->lock();
    auto offset = dirstream->offset_;
    dirstream->prefix = fh_.md_.dirent_.d_ino;
    if (dirstream->from_.empty()) {
      kvfsDirKey prfx{};
      prfx.inode_ = dirstream->prefix;
      dirstream->from_ = prfx.pack();
    }
    mutex_->unlock();

    while (dirstream->offset_ <= fh_.md_.children_offset_) {
      mutex_->lock();
      auto sr = store_->get_next_dirent(dirstream->from_, dirstream->prefix, offset);
      mutex_->unlock();
      if (sr.isValid()) {
        kvfsMetaData next_md_;
        next_md_.parse(sr);

        auto result = new kvfs_dirent();
        *result = next_md_.dirent_;
        // update offset
        mutex_->lock();
        ++dirstream->offset_;
        mutex_->unlock();
        // success
        return result;
      } else {
        // try again until offset reaches md_.children_offset.
        mutex_->lock();
        ++dirstream->offset_;
        mutex_->unlock();
        offset = dirstream->offset_;
      }
    }
    // eof
    return nullptr;
  } catch (...) {
    errorno_ = -EINTR;
    throw FSError(FSErrorType::FS_EINTR, "The ReadDir() function was interrupted by a signal.");
  }
}
int KVFS::CloseDir(kvfsDIR *dirstream) {
  // check if the fd is in open_fds
  mutex_->lock();
  kvfsFileHandle fh_;

  if (!dirstream) {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The dirp argument is a nullptr");
  }

  if (!open_fds_->find(dirstream->file_descriptor_, fh_)) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The dirp argument does not refer to an open directory stream.");
  }
  try {
    if (store_->hasKey(fh_.key_.pack())) {
      auto sr = store_->get(fh_.key_.pack());
      if (sr.isValid()) {
        kvfsMetaData check_md_;
        check_md_.parse(sr);
        if (check_md_.fstat_.st_mtim.tv_sec > fh_.md_.fstat_.st_mtim.tv_sec) {
          // file was updated in another file des recently
          // evict only
          open_fds_->evict(dirstream->file_descriptor_);
          delete dirstream;

          mutex_->unlock();
          return 0;
        }
      }
    }
    // file is new or updated
    store_->merge(fh_.key_.pack(), fh_.md_.pack());
    store_->sync();

    // release it from open_fds
    open_fds_->evict(dirstream->file_descriptor_);
    delete dirstream;

    // success
    mutex_->unlock();
    return 0;
  } catch (...) {
    errorno_ = -EINTR;
    throw FSError(FSErrorType::FS_EINTR, "The closedir() function was interrupted by a signal.");
  }
}
int KVFS::Link(const char *oldname, const char *newname) {
  return 0;
}
int KVFS::SymLink(const char *oldname, const char *newname) {
  return 0;
}
ssize_t KVFS::ReadLink(const char *filename, char *buffer, size_t size) {
  return 0;
}
int KVFS::UnLink(const char *filename) {
  return 0;
}
int KVFS::RmDir(const char *filename) {
  return 0;
}
int KVFS::Remove(const char *filename) {
  return 0;
}
int KVFS::Rename(const char *oldname, const char *newname) {
  return 0;
}
int KVFS::MkDir(const char *filename, mode_t mode) {
  auto orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    auto prv_ = pwd_;
    orig_ = pwd_.append(orig_.string());
    pwd_ = prv_;
  }
  if (orig_.is_absolute()) {
    // must start with single "/"
    if (orig_.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }

  // Attemp to resolve the real path from given path
  auto resolved = ResolvePath(orig_);

  // TODO: mkdir permission check not implemented
  // now try to make dir at that parent directory
  // so get file name of parent directory
  // get the metadata from that
  // then update its children offset
  // resolver does set current md and key to the parent of this file so use those and then update it in store.

  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
  const string &key_str = key.pack();
  // lock
  mutex_->lock();
  if (store_->hasKey(key_str)) {
    // exists return error
    errorno_ = -EEXIST;
    mutex_->unlock();
  }
  auto md_ = kvfsMetaData(resolved.first.filename(), GetFreeInode(), mode, current_key_);

  // update meta data
  md_.fstat_.st_mode |= S_IFDIR;
  md_.dirent_.d_off = resolved.second.second.children_offset_;
  ++resolved.second.second.children_offset_;
  // set this file's group id to its parent's
  md_.fstat_.st_gid = resolved.second.second.fstat_.st_gid;
  // update ctime
  md_.fstat_.st_ctim.tv_sec = time_now;
  // update parent mtime
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  // store
  store_->put(key_str, md_.pack());
  store_->merge(resolved.second.first.pack(), resolved.second.second.pack());

  //unlock
  mutex_->unlock();
  return 0;
}
int KVFS::Stat(const char *filename, struct stat *buf) {
  return 0;
}
off_t KVFS::LSeek(int filedes, off_t offset, int whence) {
  return 0;
}
ssize_t KVFS::CopyFileRange(int inputfd,
                            off64_t *inputpos,
                            int outputfd,
                            off64_t *outputpos,
                            ssize_t length,
                            unsigned int flags) {
  return 0;
}
void KVFS::Sync() {

}
int KVFS::FSync(int fildes) {
  return 0;
}
ssize_t KVFS::PRead(int filedes, void *buffer, size_t size, off_t offset) {
  // only read the file from offset argument, doesn't modify filedes offset
  kvfsFileHandle fh_;
  auto status = open_fds_->find(filedes, fh_);
  ssize_t read = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  kvfsDirKey owner = (S_ISLNK(fh_.md_.fstat_.st_mode)) ? fh_.md_.real_key_ : fh_.key_;
  // check flags first

  if ((fh_.flags_ & O_NONBLOCK) > 0) {
    // return error without writing anything
    errorno_ = -ECANCELED;
    return errorno_;
  }

  /*if ((fh_.flags_ & O_RDONLY) <= 0) {
    // file was not opened with read flag
    return 0;
  }*/

  if (offset > fh_.md_.fstat_.st_size) {
    // offset exceeds file size
    errorno_ = -E2BIG;
    throw FSError(FSErrorType::FS_EINTR, "The offset argument exceeds the filedes's file size.");
  }

  if (offset == fh_.md_.fstat_.st_size) {
    // eof
    // update stats and return
    mutex_->lock();
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->insert(filedes, fh_);
    mutex_->unlock();
    return read;
  }

  // determine where to read from and calculate blocks to read.

  size_t size_can_read = fh_.md_.fstat_.st_size > size ? size : fh_.md_.fstat_.st_size;

  if (offset < KVFS_DEF_BLOCK_SIZE) {
    // read fast from beginning
    mutex_->lock();
    std::pair<ssize_t, void *> pair = ReadBlock(&fh_.md_.inline_blck, buffer, size_can_read, offset);
    mutex_->unlock();
    read = pair.first;
    size_t size_left = size_can_read - read;
    size_t blocks_to_read_ = size_left / KVFS_DEF_BLOCK_SIZE;
    blocks_to_read_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
    read += ReadBlocks(fh_.md_.inline_blck.next_block_, blocks_to_read_, size_left, read > 0 ? 0 : offset, pair.second);

    // finished
    // update stats and return
    mutex_->lock();
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->insert(filedes, fh_);
    mutex_->unlock();
    return read;
  }

  return read;
  /*auto blk_offset = static_cast<uint64_t>(offset / KVFS_DEF_BLOCK_SIZE);
  // seek block at current offset from store
  kvfsBlockKey key{};
  key.offset_ = blk_offset;
  mutex_->lock();
  store_->seek_at(key.pack(), blk_offset, owner);
  mutex_->unlock();*/
}
ssize_t KVFS::PWrite(int filedes, const void *buffer, size_t size, off_t offset) {
  return 0;
}
int KVFS::ChMod(const char *filename, mode_t mode) {
  return 0;
}
int KVFS::Access(const char *filename, int how) {
  return 0;
}
int KVFS::UTime(const char *filename, const struct utimbuf *times) {
  return 0;
}
int KVFS::Truncate(const char *filename, off_t length) {
  return 0;
}
int KVFS::Mknod(const char *filename, mode_t mode, dev_t dev) {
  return 0;
}
void KVFS::TuneFS() {
  store_->compact();

  store_->sync();
}
void KVFS::DestroyFS() {
  if (!store_->destroy()) {
    throw FSError(FSErrorType::FS_EIO, "Failed to destroy store");
  }
  store_.reset();
  inode_cache_.reset();
  open_fds_.reset();

  std::filesystem::remove_all(root_path);

}
uint32_t KVFS::GetFreeFD() {
  if (next_free_fd_ == KVFS_MAX_OPEN_FILES) {
    // error too many open files
    errorno_ = -ENFILE;
    throw FSError(FSErrorType::FS_ENFILE, "Too many files are currently open in the system.");
  } else {
    auto fi = next_free_fd_;
    ++next_free_fd_;
    return fi;
  }
}
std::pair<ssize_t, kvfs::kvfsBlockKey> KVFS::WriteBlocks(kvfsBlockKey blck_key_,
                                                         size_t blcks_to_write_,
                                                         const void *buffer,
                                                         size_t buffer_size_,
                                                         const kvfsDirKey &owner) {
  auto batch = store_->get_write_batch(blcks_to_write_);
  const void *idx = buffer;
  ssize_t written = 0;
  for (size_t i = 0; i < blcks_to_write_; ++i) {
    auto *bv_ = new kvfsBlockValue();
    auto pair = FillBlock(bv_, idx, buffer_size_, owner);

    // if buffer size is is still big get another block
    if (buffer_size_ > KVFS_DEF_BLOCK_SIZE) {
      bv_->next_block_ = GetFreeBlock();
      bv_->next_block_.offset_ = blck_key_.offset_ + KVFS_DEF_BLOCK_SIZE;
    }
#ifdef KVFS_DEBUG
    std::cout << blck_key_.block_number_ << " " << blck_key_.offset_ << std::endl;
#endif
    batch->put(blck_key_.pack(), bv_->pack());
    buffer_size_ -= pair.first;
    written += pair.first;
    // update offset
    idx = pair.second;
    blck_key_ = bv_->next_block_;
    delete (bv_);
  }
  // flush the write batch
  batch->flush();
  auto result = std::pair<ssize_t, kvfs::kvfsBlockKey>();
  result.first = written;
  result.second = blck_key_;
  return result;
}
std::pair<size_t, const void *> KVFS::FillBlock(kvfsBlockValue *blck_,
                                                const void *buffer,
                                                size_t buffer_size_,
                                                const kvfsDirKey &owner) {
  size_t write_size = ((buffer_size_ <= KVFS_DEF_BLOCK_SIZE))
                      ? (buffer_size_) : (KVFS_DEF_BLOCK_SIZE);
  auto idx = blck_->write(buffer, write_size);
  blck_->owener_ = owner;
  auto pair = std::pair<size_t, const void *>();
  pair.first = write_size;
  pair.second = idx;
  return pair;
}
ssize_t KVFS::ReadBlocks(kvfsBlockKey blck_key_,
                         size_t blcks_to_read_,
                         size_t buffer_size_,
                         off_t offset,
                         void *buffer) {
  void *idx = buffer;
  ssize_t read = 0;
  for (size_t i = 0; i < blcks_to_read_; ++i) {
    mutex_->lock();
    auto sr = store_->get(blck_key_.pack());
    mutex_->unlock();
    if (sr.isValid()) {
      kvfsBlockValue bv_;
      bv_.parse(sr);
#ifdef KVFS_DEBUG
      std::cout << blck_key_.block_number_ << " " << blck_key_.offset_ << std::endl;
#endif
      auto pair = ReadBlock(&bv_, idx, buffer_size_, offset);
      if (pair.first > 0) {
        offset = 0;
      }
      idx = pair.second;
      read += pair.first;
      blck_key_ = bv_.next_block_;
      buffer_size_ -= pair.first;
    }
  }
  return read;
}
std::pair<ssize_t, void *> KVFS::ReadBlock(kvfsBlockValue *blck_, void *buffer, size_t size, off_t offset) {
  // if offset is between 0 and KVFSblocksize then start from there otherwise return with size 0 and buffer
  // calculate size to read, if size is smaller than kvfs blck size then read upto size = (kvfs blck size - offset)
  // else size = size
  if (offset >= KVFS_DEF_BLOCK_SIZE) {
    return std::pair<ssize_t, void *>(0, buffer);
  }
  auto max_readable_size = static_cast<size_t>(KVFS_DEF_BLOCK_SIZE - offset);
  void *idx = buffer;
  if (size < max_readable_size) {
    idx = blck_->read_at(buffer, size, offset);
    return std::pair<ssize_t, void *>(size, idx);
  } else {
    idx = blck_->read_at(buffer, max_readable_size, offset);
    return std::pair<ssize_t, void *>(max_readable_size, idx);
  }
}

}  // namespace kvfs