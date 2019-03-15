/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvfs.cpp
 */

#include <kvfs/kvfs.h>
#include <utime.h>

namespace kvfs {
kvfs::KVFS::KVFS(const std::string &mount_path)
    : root_path(mount_path),
#if KVFS_HAVE_ROCKSDB
    store_(std::make_shared<RocksDBStore>(mount_path)),
#endif
#if KVFS_HAVE_LEVELDB
      store_(std::make_shared<kvfsLevelDBStore>(mount_path)),
#endif
//      inode_cache_(std::make_unique<InodeCache>(KVFS_MAX_OPEN_FILES, store_)),
      open_fds_(std::make_unique<DentryCache>(512)),
      cwd_name_(""),
      pwd_("/"),
      errorno_(0),
      current_key_{},
      current_md_{},
      next_free_fd_(0)
#if KVFS_THREAD_SAFE
,mutex_(std::make_unique<std::mutex>())
#endif
{
  FSInit();
}
kvfs::KVFS::KVFS()
    : root_path("/tmp/db/"),
#if KVFS_HAVE_ROCKSDB
    store_(std::make_shared<RocksDBStore>(root_path)),
#endif
#if KVFS_HAVE_LEVELDB
      store_(std::make_shared<kvfsLevelDBStore>(root_path)),
#endif
//      inode_cache_(std::make_unique<InodeCache>(512, store_)),
      open_fds_(std::make_unique<DentryCache>(512)),
      cwd_name_(""),
      pwd_("/"),
      errorno_(0),
      current_key_{},
      current_md_{},
      next_free_fd_(0)
#if KVFS_THREAD_SAFE
,mutex_(std::make_unique<std::mutex>())
#endif
{
  FSInit();
}

kvfs::KVFS::~KVFS() {
#if KVFS_THREAD_SAFE
  mutex_.reset();
#endif
//  inode_cache_.reset();
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
int kvfs::KVFS::ChDir(const char *path) {
  std::filesystem::path orig_ = std::filesystem::path(path);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // retrieve the filename from store, if it doesn't exist then error
  kvfsDirKey key_ = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (sr.isValid()) {
    // found the file, check if it is a directory
    kvfsMetaData md_;
    md_.parse(sr);
    if (S_ISLNK(md_.fstat_.st_mode)) {
      // it is symbolic link get the real metadata
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      StoreResult sr = store_->get(key_.pack());
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
      if (sr.isValid()) {
        md_.parse(sr);
      }
    }
    if (!S_ISDIR(md_.fstat_.st_mode)) {
      errorno_ = -ENOTDIR;
      throw FSError(FSErrorType::FS_ENOTDIR, "A component of the pathname names an existing file that is neither "
                                             "a directory nor a symbolic link to a directory.");
    }
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    current_md_ = md_;
    current_key_ = key_;
    cwd_name_ = resolved.first.filename();
    pwd_ = resolved.first;
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return 0;
  }
  errorno_ = -ENONET;
  return errorno_;
}
kvfsDIR *KVFS::OpenDir(const char *path) {
  std::filesystem::path orig_ = std::filesystem::path(path);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

#if KVFS_THREAD_SAFE
  // lock
    mutex_->lock();
#endif
  kvfs_file_hash_t hash = 0;

  if (resolved.first != "/") {
    hash = std::filesystem::hash_value(resolved.first.filename());
  } else {
    hash = std::filesystem::hash_value(resolved.first);
  }

  kvfsDirKey key = {resolved.second.first.inode_, hash};
  const std::string &key_str = key.pack();
  StoreResult sr = store_->get(key_str);
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
    uint32_t fd_ = GetFreeFD();
    kvfsFileHandle fh_ = kvfsFileHandle();
    fh_.md_ = md_;
    fh_.key_ = key;

    open_fds_->insert(fd_, fh_);

#if KVFS_THREAD_SAFE
    //unlock
        mutex_->unlock();
#endif
    //success
    kvfsDIR *result = new kvfsDIR();
    result->file_descriptor_ = fd_;
    result->ptr_ = store_->get_iterator();
    kvfsDirKey seek_key = {md_.fstat_.st_ino, 0};
    result->ptr_->Seek(seek_key.pack());
    return result;
  }
  if (!sr.isValid()) {
    // does not exist return error
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "A component of dirname does not name an existing directory "
                                          "or dirname is an empty string.");
  }
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return nullptr;
}
int kvfs::KVFS::Open(const char *filename, int flags, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // now the path components all exist and resolved
  if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
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

  const std::string &key_str = key.pack();
  if (flags & O_CREAT) {
    // If set, the file will be created if it doesn’t already exist.
    if (flags & O_EXCL) {
      // If both O_CREAT and O_EXCL are set, then open fails if the specified file already exists.
      // This is guaranteed to never clobber an existing file.
      {
#if KVFS_THREAD_SAFE
        mutex_->lock();
#endif
        if (store_->get(key_str).isValid()) {
          // file exists return error
#if KVFS_THREAD_SAFE
          mutex_->unlock();
#endif
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
      std::filesystem::path name = resolved.first.filename().string();
      kvfsMetaData md_ = kvfsMetaData(name, GetFreeInode(), mode, current_key_);
#if KVFS_THREAD_SAFE
      // Acquire lock
            mutex_->lock();
#endif

      // generate a file descriptor
      kvfsFileHandle fh_ = kvfsFileHandle(key, md_, flags);
      uint32_t fd_ = next_free_fd_;
      ++next_free_fd_;

      // insert into store
      store_->put(key_str, md_.pack());
      // write it back if flag O_SYNC
      if (flags & O_SYNC) {
        store_->sync();
      }
      // add it to open_fds
      open_fds_->insert(fd_, fh_);

#if KVFS_THREAD_SAFE
      // release lock
      mutex_->unlock();
#endif
      // update the parent
      ++resolved.second.second.fstat_.st_nlink;
      resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
      bool status2 = store_->put(resolved.second.first.pack(), resolved.second.second.pack());

      return fd_;
    }
  }

  // flag is not O_CREAT so open existing file
#if KVFS_THREAD_SAFE
  // Acquire lock
    mutex_->lock();
#endif

  StoreResult sr = store_->get(key_str);
  // ensure sr is valid
  if (!sr.isValid()) {
    errorno_ = -EIO;
    return errorno_;
  }
  kvfsMetaData md_ = kvfsMetaData();
  md_.parse(sr);
  kvfsFileHandle fh_ = kvfsFileHandle(key, md_, flags);

  uint32_t fd_ = next_free_fd_;
  ++next_free_fd_;

  // add it to open_fds
  open_fds_->insert(fd_, fh_);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif

  return fd_;
}
void kvfs::KVFS::FSInit() {
  if (store_ != nullptr) {
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    StoreResult sb = store_->get("superblock");
      if (sb.isValid()) {
        super_block_.parse(sb);
        super_block_.fs_number_of_mounts_++;
        super_block_.fs_last_mount_time_ = time_now;
      }
     else {
      super_block_.fs_creation_time_ = time_now;
      super_block_.fs_last_mount_time_ = time_now;
      super_block_.fs_number_of_mounts_ = 1;
      super_block_.total_block_count_ = 0;
      super_block_.total_inode_count_ = 1;
      super_block_.next_free_inode_ = 1;
      bool status = store_->put("superblock", super_block_.pack());
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to IO with store");
      }
    }
    kvfsDirKey root_key = {0, std::filesystem::hash_value("/")};
    if (!store_->get(root_key.pack()).isValid()) {
      mode_t mode = geteuid() | getegid() | S_IFDIR;
      kvfsMetaData root_md = kvfsMetaData("/", 0, mode, root_key);
      store_->put(root_key.pack(), root_md.pack());
    }
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
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
  std::filesystem::path output;
  kvfs_file_inode_t inode = 0;
  int invalid_count = 0;
  kvfsDirKey parent_key_;
  kvfsMetaData parent_md_;
  std::string key_str;
  kvfsMetaData md_;
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
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      kvfsDirKey key = {inode, std::filesystem::hash_value(e)};
      key_str = key.pack();
      StoreResult sr = store_->get(key_str);
      if (sr.isValid()) {
        // found the key in store
        md_.parse(sr);
        bool is_link = S_ISLNK(md_.fstat_.st_mode);
        if (is_link) {
          // we occurred a symbolic link, prefix the path with the contents
          // of this link
#if KVFS_THREAD_SAFE
          mutex_->unlock();
#endif
          output = GetSymLinkContentsPath(md_);
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
        ++invalid_count;
      }
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
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
std::filesystem::path kvfs::KVFS::GetSymLinkContentsPath(const kvfs::kvfsMetaData &data) {
  std::filesystem::path output;
  std::list<std::filesystem::path> path_list;
  kvfsBlockKey blck_key = {data.fstat_.st_ino, 0};
  errorno_ = 0;
  // read contents of the symlink block and convert it to a path
  void *buffer = malloc(static_cast<size_t>(data.fstat_.st_size));
  size_t blcks_to_read = static_cast<size_t>(data.last_block_key_.block_number_);
  ssize_t read = ReadBlocks(blck_key, blcks_to_read, static_cast<size_t>(data.fstat_.st_size), 0, buffer);
  output = (char *) buffer;
  free(buffer);
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
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  if (super_block_.freeblocks_count_ < 512) {
    // add it to first freeblocks block
    kvfsFreeBlocksKey fb_key = {"fb", 0};

      StoreResult sr = store_->get(fb_key.pack());
      if (sr.isValid()) {
        kvfsFreeBlocksValue value{};
        value.parse(sr);
        value.blocks[value.count_] = key;
        ++value.count_;
        // merge it in store
        bool status = store_->merge(fb_key.pack(), value.pack());
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
    else {
      // generate the first fb block
      kvfsFreeBlocksValue value{};
      value.count_ = 1;
      ++super_block_.freeblocks_count_;
      value.blocks[0] = key;
      // put in store
      bool status = store_->put(fb_key.pack(), value.pack());
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
    bool status = store_->merge(fb_key.pack(), fb_value.pack());
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
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return true;
}
ssize_t kvfs::KVFS::Read(int filedes, void *buffer, size_t size) {
  // read from offset in the file descriptor
  // if offset is at eof then return 0 else try to pread with filedes offset
  kvfsFileHandle fh_;
  bool status = open_fds_->find(filedes, fh_);
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }

  // check flags first

  if (fh_.offset_ == fh_.md_.fstat_.st_size) {
    // eof

    // update stats and return
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->evict(filedes);
    open_fds_->insert(filedes, fh_);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return 0;
  } else {
    // try to read the file at the file descriptor position
    // then modify filedes offset by amount read
    ssize_t read = PRead(filedes, buffer, size, fh_.offset_);
    fh_.offset_ += read;
    open_fds_->evict(filedes);
    open_fds_->insert(filedes, fh_);
    return read;
  }
}
ssize_t kvfs::KVFS::Write(int filedes, const void *buffer, size_t size) {
  kvfsFileHandle fh_;
  bool status = open_fds_->find(filedes, fh_);
  ssize_t written = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first

  // check if O_APPEND is set, then always append
  bool append_only = (fh_.flags_ & O_APPEND) > 0;
  if (append_only) {
    // ignore offset and append only

    // determine which block to write from, then
    // pass that to blocks writer.

    kvfsBlockKey blck_key_;
    blck_key_.inode_ = fh_.md_.fstat_.st_ino;
    blck_key_.block_number_ = fh_.md_.fstat_.st_size / KVFS_DEF_BLOCK_SIZE;
    kvfs_off_t blck_offset = fh_.md_.fstat_.st_size % KVFS_DEF_BLOCK_SIZE;
    const void *idx = buffer;

    StoreResult sr = store_->get(blck_key_.pack());
    if (sr.isValid()) {
      kvfsBlockValue bv_;
      bv_.parse(sr);
      auto pair = FillBlock(&bv_, buffer, size, blck_offset);
      written += pair.first;
      idx = pair.second;
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      store_->put(blck_key_.pack(), bv_.pack());
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
      blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
    }

    size_t size_left = size - written;
    size_t blocks_to_write_ = (size_left) / KVFS_DEF_BLOCK_SIZE;
    blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
    std::pair<ssize_t, kvfs::kvfsBlockKey> result =
        WriteBlocks(blck_key_, blocks_to_write_, idx, size_left, 0);
    written += result.first;

    // update stats
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    fh_.md_.last_block_key_ = result.second;
    fh_.md_.fstat_.st_size += written;
    fh_.offset_ += written;
    fh_.md_.fstat_.st_mtim.tv_sec = time_now;
    // update this filedes in cache
    open_fds_->evict(filedes);
    open_fds_->insert(filedes, fh_);
    // finished
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return written;
  } else {
    // call to PWrite to write from offset
    written = PWrite(filedes, buffer, size, fh_.offset_);
    fh_.offset_ += written;
    open_fds_->evict(filedes);
    open_fds_->insert(filedes, fh_);
    return written;
  }
}
kvfs::kvfsBlockKey kvfs::KVFS::GetFreeBlock() {
  // search free block first
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  if (super_block_.freeblocks_count_ != 0) {
    size_t fb_number = super_block_.freeblocks_count_ / 512;
    kvfsFreeBlocksKey fb_key = {"fb", fb_number};
    StoreResult sr = store_->get(fb_key.pack());
    if (sr.isValid()) {
      kvfsFreeBlocksValue val{};
      val.parse(sr);
      kvfsBlockKey key = val.blocks[val.count_ - 1];

      // check if the block refers to another block
      kvfsBlockValue bv_{};
      kvfsBlockKey in_key_ = key;
      sr = store_->get(in_key_.pack());
        if (sr.isValid()) {
          bv_.parse(sr);
          if (bv_.next_block_.block_number_ != 0) {
            in_key_ = bv_.next_block_;
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
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
      return key;
    }
  }
  // get a free block from superblock
  kvfsBlockKey bk_;
  ++super_block_.next_free_block_number;
  ++super_block_.total_block_count_;
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return bk_;
}
int KVFS::Close(int filedes) {
  // check filedes exists
  try {
    kvfsFileHandle fh_;
#if KVFS_THREAD_SAFE
    mutex_->lock(); // lock
#endif
    bool status = open_fds_->find(filedes, fh_);
    if (!status) {
      errorno_ = -EBADFD;
      return errorno_;
    }
    // update store
    // check if file exists in the store
    // if file is in store, check it's mtime
    // if it matches this file des then update it otherwise just evict this file des
    // if it doesn't exist then assume it is a new file
    StoreResult sr = store_->get(fh_.key_.pack());
      if (sr.isValid()) {
        kvfsMetaData check_md_;
        check_md_.parse(sr);
        if (check_md_.fstat_.st_mtim.tv_sec > fh_.md_.fstat_.st_mtim.tv_sec) {
          // file was updated in another file des recently
          // evict only
          open_fds_->evict(filedes);
#if KVFS_THREAD_SAFE
          mutex_->unlock(); // unlock
#endif
          return 0;
        }
      }


    if (fh_.flags_ & O_CREAT) {
      // check flags and see if it was opened with O_CREAT
        // file is new or updated
        store_->merge(fh_.key_.pack(), fh_.md_.pack());
        store_->sync();
        // release it from open_fds
        open_fds_->evict(filedes);
        // success
#if KVFS_THREAD_SAFE
        mutex_->unlock(); //unlock
#endif
        return 0;
    }

    // file is neither new or exists in store anymore
    // so just evict and return
    open_fds_->evict(filedes);
    // success
#if KVFS_THREAD_SAFE
    mutex_->unlock(); //unlock
#endif
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
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  if (!open_fds_->find(dirstream->file_descriptor_, fh_)) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The dirp argument does not refer to an open directory stream.");
  }
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  try {
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    if (dirstream->ptr_->Valid()) {
      dirstream->ptr_->Next();
      if (dirstream->ptr_->key().size() == sizeof(kvfsDirKey)
          && dirstream->ptr_->value().asString().size() == sizeof(kvfsMetaData)) {
        kvfsDirKey key;
        key.parse(dirstream->ptr_->key());
        kvfsMetaData md_;
        md_.parse(dirstream->ptr_->value());

        if (key.inode_ == fh_.md_.fstat_.st_ino) {
          kvfs_dirent *result = new kvfs_dirent();
          *result = md_.dirent_;
#if KVFS_THREAD_SAFE
          mutex_->unlock();
#endif
          return result;
        }
      }
    }
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    // eof
    return nullptr;
  } catch (...) {
    errorno_ = -EINTR;
    throw FSError(FSErrorType::FS_EINTR, "The ReadDir() function was interrupted by a signal.");
  }
}
int KVFS::CloseDir(kvfsDIR *dirstream) {
  // check if the fd is in open_fds
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
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
    StoreResult sr = store_->get(fh_.key_.pack());
      if (sr.isValid()) {
        kvfsMetaData check_md_;
        check_md_.parse(sr);
        if (check_md_.fstat_.st_mtim.tv_sec > fh_.md_.fstat_.st_mtim.tv_sec) {
          // file was updated in another file des recently
          // evict only
          open_fds_->evict(dirstream->file_descriptor_);
          dirstream->ptr_.reset();
          delete dirstream;
#if KVFS_THREAD_SAFE
          mutex_->unlock();
#endif
          return 0;
        }
      }

    // file is new or updated
    store_->merge(fh_.key_.pack(), fh_.md_.pack());
    store_->sync();

    // release it from open_fds
    open_fds_->evict(dirstream->file_descriptor_);
    dirstream->ptr_.reset();
    delete dirstream;

    // success
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return 0;
  } catch (...) {
    errorno_ = -EINTR;
    throw FSError(FSErrorType::FS_EINTR, "The closedir() function was interrupted by a signal.");
  }
}
int KVFS::Link(const char *oldname, const char *newname) {

  std::filesystem::path orig_old = std::filesystem::path(oldname);
  std::filesystem::path orig_new = std::filesystem::path(oldname);
  if (orig_old.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
    orig_old = pwd_.append(orig_old.string());
    pwd_ = prv_;
  }
  if (orig_old.is_absolute()) {
    // must start with single "/"
    if (orig_old.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }
  if (orig_new.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
    orig_new = pwd_.append(orig_new.string());
    pwd_ = prv_;
  }
  if (orig_new.is_absolute()) {
    // must start with single "/"
    if (orig_new.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }

  // Attemp to resolve the real path from given path
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved_old = RealPath(orig_old);
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved_new = RealPath(orig_new);

  // create a new link (directory entry) for the existing file

  // check if old name exists

  kvfsDirKey
      old_key = {resolved_old.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved_old.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(old_key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "A component of either path prefix does not exist; "
                                          "the file named by path1 does not exist; or path1 or path2 points to an empty string.");
  }
  kvfsMetaData old_md_;
  old_md_.parse(sr);
  kvfsDirKey
      new_key = {resolved_new.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved_new.first.filename())};
  // set the inode of this new link to the inode of the original file
  kvfsMetaData new_md_ = kvfsMetaData(resolved_new.first.filename(), old_md_.fstat_.st_ino,
                                      resolved_new.second.second.fstat_.st_mode, resolved_new.second.first);
  if (old_md_.real_key_ != old_key) {
    // check if original exists, if it doesn't exist then make the old one original
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    StoreResult sr = store_->get(old_md_.real_key_.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (!sr.isValid()) {
      old_md_.real_key_ = old_key;
      --old_md_.fstat_.st_nlink;
    } else {
      kvfsMetaData original_md_;
      original_md_.parse(sr);
      ++original_md_.fstat_.st_nlink;
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      store_->merge(old_md_.real_key_.pack(), original_md_.pack());
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
    }
  }
  new_md_.real_key_ = old_md_.real_key_; // set real key to old key, needed to resolve links and avoid infinite loops
  ++new_md_.fstat_.st_nlink;
  ++old_md_.fstat_.st_nlink;
  old_md_.fstat_.st_atim.tv_sec = time_now;
  old_md_.fstat_.st_mtim.tv_sec = time_now;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status1 = store_->merge(old_key.pack(), old_md_.pack());
  bool status2 = store_->put(new_key.pack(), new_md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1 & status2;
}
int KVFS::SymLink(const char *path1, const char *path2) {
  std::filesystem::path orig_path2 = std::filesystem::path(path1);
  if (orig_path2.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
    orig_path2 = pwd_.append(orig_path2.string());
    pwd_ = prv_;
  }
  if (orig_path2.is_absolute()) {
    // must start with single "/"
    if (orig_path2.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved_path_2 = RealPath(orig_path2);
  // The symlink() function shall create a symbolic link called path2 that contains the string pointed to by path1
  // (path2 is the name of the symbolic link created, path1 is the string contained in the symbolic link).
  //The string pointed to by path1 shall be treated only as a string and shall not be validated as a pathname.

  kvfsDirKey slkey_ =
      {resolved_path_2.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved_path_2.first.filename())};
  // check if the key names an existing file
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(slkey_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (sr.isValid()) {
    // return error EEXISTS
    errorno_ = -EEXIST;
    throw FSError(FSErrorType::FS_EEXIST, "The path2 argument names an existing file.");
  }
  // create the symlink
  kvfsMetaData slmd_ =
      kvfsMetaData(resolved_path_2.first.filename(),
                   GetFreeInode(),
                   S_IFLNK | resolved_path_2.second.second.fstat_.st_mode,
                   resolved_path_2.second.first);

  kvfsBlockKey sl_blck_key = kvfsBlockKey(slmd_.fstat_.st_ino, 0);
  size_t blcks_to_write = strlen(path1) / KVFS_DEF_BLOCK_SIZE;
  blcks_to_write += (strlen(path1) % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  auto pair = WriteBlocks(sl_blck_key, blcks_to_write, path1, strlen(path1), 0);
  slmd_.last_block_key_ = pair.second;
  slmd_.fstat_.st_size = pair.first;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status1 = store_->put(slkey_.pack(), slmd_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1;
}
ssize_t KVFS::ReadLink(const char *filename, char *buffer, size_t size) {
  // The readlink() function shall place the contents of the symbolic link referred to by path in the buffer buf which
  // has size bufsize. If the number of bytes in the symbolic link is less than bufsize, the contents of the remainder
  // of buf are unspecified. If the buf argument is not large enough to contain the link content, the first bufsize
  // bytes shall be placed in buf.

  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
    orig_ = pwd_.append(orig_.string());
    pwd_ = prv_;
  }
  if (orig_.is_absolute()) {
    // must start with single "/"
    if (orig_.root_path() != "/") {
      throw FSError(FSErrorType::FS_EINVAL, "Given name is not in the correct format");
    }
  }
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
  // check if the key names an existing file
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  // check if it is a symlink
  if (!S_ISLNK(md_.fstat_.st_mode)) {
    // not a symlink
    // return error
    errorno_ = -ENOLINK;
    return errorno_;
  }
  kvfsBlockKey blck_key_ = kvfsBlockKey(md_.fstat_.st_ino, 0);
  ssize_t size_to_read = (size > md_.fstat_.st_size ? md_.fstat_.st_size : size);
  size_t blcks_to_read = size_to_read / KVFS_DEF_BLOCK_SIZE;
  blcks_to_read += (size_to_read % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  ssize_t pair = ReadBlocks(blck_key_, blcks_to_read, size, 0, buffer);
  return pair;
}
int KVFS::UnLink(const char *filename) {
  // decrease file's link count by one, if it reaches zero then delete the file from store
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);
  if (resolved.first.filename() == "." || resolved.first.filename() == "..") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The path argument contains a last component that is dot.");
  }
  if (resolved.first == "/") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The path argument refers to the root directory,"
                                          " it is not possible to remove root directory.");
  }
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENOENT;
    return errorno_;
  }
  // found the file, check if it is a directory
  kvfsMetaData md_;
  md_.parse(sr);
  if (S_ISLNK(md_.fstat_.st_mode)) {
    // just delete it
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->delete_(key.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return status;
  }
  // decrease link count
  --md_.fstat_.st_nlink;
  if (md_.fstat_.st_nlink <= 0) {
    // remove it from store
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->delete_(key.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return status;
  }
  md_.fstat_.st_mtim.tv_sec = time_now;
  // update it in store
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status1 = store_->put(key.pack(), md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif

  // update the parent
  --resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status2 = store_->put(resolved.second.first.pack(), resolved.second.second.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1 & status2;
}
int KVFS::RmDir(const char *filename) {
  return this->UnLink(filename);
}
int KVFS::Remove(const char *filename) {
  return this->UnLink(filename);
}
int KVFS::Rename(const char *oldname, const char *newname) {
  std::filesystem::path oldname_orig = std::filesystem::path(oldname);
  std::filesystem::path newname_orig = std::filesystem::path(newname);
  // check each for parents to exist, then rename
  if (oldname_orig.is_relative()) {
    std::filesystem::path prv_ = pwd_;
    oldname_orig = pwd_.append(oldname_orig.string());
    pwd_ = prv_;
  }
  if (newname_orig.is_relative()) {
    std::filesystem::path prv_ = pwd_;
    newname_orig = pwd_.append(newname_orig.string());
    pwd_ = prv_;
  }
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> oldname_resolved = RealPath(oldname_orig);
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> newname_resolved = RealPath(newname_orig);
  // check if oldname exists and newname doesn't exist
  kvfsDirKey old_key =
      {oldname_resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(oldname_resolved.first.filename())};
  kvfsDirKey new_key =
      {newname_resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(newname_resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult old_sr = store_->get(old_key.pack());
  bool new_exists = store_->get(new_key.pack()).isValid();
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!old_sr.isValid()) {
    errorno_ = -ENOENT;
    throw FSError(FSErrorType::FS_ENOENT, "The oldname argument doesn't name a existing entry");
  }
  if (new_exists) {
    errorno_ = -EEXIST;
    throw FSError(FSErrorType::FS_EEXIST, "The newname argument already exists");
  }
  auto batch = store_->get_write_batch();
  // decrease link count on old name's parent
  oldname_resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  --oldname_resolved.second.second.fstat_.st_nlink;
  batch->delete_(old_key.pack());
  batch->delete_(oldname_resolved.second.first.pack());
  batch->put(oldname_resolved.second.first.pack(), oldname_resolved.second.second.pack());
  // copy over old metadata under new key
  kvfsMetaData new_md_;
  new_md_.parse(old_sr);
  std::string new_name = newname_resolved.first.filename();
  kvfs_dirent new_dirent{};
  new_dirent.d_ino = new_md_.dirent_.d_ino;
  new_name.copy(new_dirent.d_name, new_name.size(), 0);
  new_md_.dirent_ = new_dirent;
  batch->put(new_key.pack(), new_md_.pack());
  batch->flush();
  batch.reset();
  return 0;
}
int KVFS::MkDir(const char *filename, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // now try to make dir at that parent directory
  // so get file name of parent directory
  // get the metadata from that
  // then update its children offset
  // resolver does set current md and key to the parent of this file so use those and then update it in store.

  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
  const std::string &key_str = key.pack();
#if KVFS_THREAD_SAFE
  // lock
  mutex_->lock();
#endif
  if (store_->get(key_str).isValid()) {
    // exists return error
    errorno_ = -EEXIST;
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
  }
  kvfsMetaData md_ = kvfsMetaData(resolved.first.filename(),
                                  GetFreeInode(),
                                  mode | resolved.second.second.fstat_.st_mode,
                                  resolved.second.first);

  // update meta data
  md_.fstat_.st_mode |= S_IFDIR;
  // set this file's group id to its parent's
  md_.fstat_.st_gid = resolved.second.second.fstat_.st_gid;
  // update ctime
  md_.fstat_.st_ctim.tv_sec = time_now;
  // update it in store
  bool status1 = store_->put(key.pack(), md_.pack());
  // update the parent
  ++resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  bool status2 = store_->put(resolved.second.first.pack(), resolved.second.second.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1 & status2;
}
int KVFS::Stat(const char *filename, kvfs_stat *buf) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // check for existing file
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  *buf = md_.fstat_;
  return 0;
}
off_t KVFS::LSeek(int filedes, off_t offset, int whence) {
  kvfsFileHandle fh_;
  bool status = open_fds_->find(filedes, fh_);
  ssize_t read = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }

  //  If whence is SEEK_SET, the file offset shall be set to offset bytes.
  //  If whence is SEEK_CUR, the file offset shall be set to its current location plus offset.
  //  If whence is SEEK_END, the file offset shall be set to the size of the file plus offset.

  kvfs_off_t result;
  if (whence == SEEK_SET) {
    fh_.offset_ = offset;
  }
  if (whence == SEEK_CUR) {
    fh_.offset_ += offset;
  }
  if (whence == SEEK_END) {
    fh_.md_.fstat_.st_size += offset;
  }
  result = fh_.offset_;
  open_fds_->evict(filedes);
  open_fds_->insert(filedes, fh_);

  return result;
}
ssize_t KVFS::CopyFileRange(int inputfd,
                            off64_t *inputpos,
                            int outputfd,
                            off64_t *outputpos,
                            ssize_t length,
                            unsigned int flags) {
  // not implemented
  return -1;
}
void KVFS::Sync() {
  store_->sync();
}
int KVFS::FSync(int filedes) {
  // currently only same as sync
  this->Sync();
  return 0;
}
ssize_t KVFS::PRead(int filedes, void *buffer, size_t size, off_t offset) {
  // only read the file from offset argument, doesn't modify filedes offset
  kvfsFileHandle fh_;
  bool status = open_fds_->find(filedes, fh_);
  ssize_t read = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first

  /*if ((fh_.flags_ & O_RDONLY) <= 0) {
    // file was not opened with read flag
    return 0;
  }*/

  if (offset > fh_.md_.fstat_.st_size) {
    // offset exceeds file size
    return 0;
  }

  if (offset == fh_.md_.fstat_.st_size) {
    // eof
    // update stats and return
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->insert(filedes, fh_);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return read;
  }

  // determine where to read from and calculate blocks to read.

  size_t size_can_read = fh_.md_.fstat_.st_size > size ? size : fh_.md_.fstat_.st_size;

  // calculate block key to read from and read upto size_can_read

  kvfsBlockKey blck_key_;
  blck_key_.inode_ = fh_.md_.fstat_.st_ino;
  blck_key_.block_number_ = offset / KVFS_DEF_BLOCK_SIZE;
  kvfs_off_t blck_offset = offset % KVFS_DEF_BLOCK_SIZE;
  void *idx = buffer;

  StoreResult sr = store_->get(blck_key_.pack());
  if (sr.isValid()) {
    kvfsBlockValue bv_;
    bv_.parse(sr);
    auto pair = ReadBlock(&bv_, buffer, size, blck_offset);
    read += pair.first;
    idx = pair.second;
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    store_->put(blck_key_.pack(), bv_.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
  }

  size_can_read -= read;
  size_t blocks_to_read_ = size_can_read / KVFS_DEF_BLOCK_SIZE;
  blocks_to_read_ += ((size_can_read % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
  read += ReadBlocks(blck_key_, blocks_to_read_, size_can_read, 0, idx);
  // finished
  // update stats and return
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  if (!(fh_.flags_ & O_NOATIME)) {
    // update accesstimes on the file
    fh_.md_.fstat_.st_atim.tv_sec = time_now;
  }
  open_fds_->insert(filedes, fh_);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return read;
}

ssize_t KVFS::PWrite(int filedes, const void *buffer, size_t size, off_t offset) {
  kvfsFileHandle fh_;
  bool status = open_fds_->find(filedes, fh_);
  ssize_t written = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADF, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first

  if (offset > fh_.md_.fstat_.st_size) {
    // offset exceeds file size
    errorno_ = -E2BIG;
    throw FSError(FSErrorType::FS_EINTR, "The offset argument exceeds the filedes's file size.");
  }

  if (offset == fh_.md_.fstat_.st_size) {
    // eof
    // update stats and return
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    if (!(fh_.flags_ & O_NOATIME)) {
      // update accesstimes on the file
      fh_.md_.fstat_.st_atim.tv_sec = time_now;
    }
    open_fds_->insert(filedes, fh_);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return written;
  }

  // determine which block to write from, then
  // pass that to blocks writer.

  kvfsBlockKey blck_key_;
  blck_key_.inode_ = fh_.md_.fstat_.st_ino;
  blck_key_.block_number_ = offset / KVFS_DEF_BLOCK_SIZE;
  kvfs_off_t blck_offset = offset % KVFS_DEF_BLOCK_SIZE;
  const void *idx = buffer;

  StoreResult sr = store_->get(blck_key_.pack());
  if (sr.isValid()) {
    kvfsBlockValue bv_;
    bv_.parse(sr);
    auto pair = FillBlock(&bv_, buffer, size, blck_offset);
    written += pair.first;
    idx = pair.second;
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    store_->put(blck_key_.pack(), bv_.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
  }

  size_t size_left = size - written;
  size_t blocks_to_write_ = (size_left) / KVFS_DEF_BLOCK_SIZE;
  blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
  std::pair<ssize_t, kvfs::kvfsBlockKey> result =
      WriteBlocks(blck_key_, blocks_to_write_, idx, size_left, 0);
  written += result.first;

  // update stats
  fh_.md_.last_block_key_ = result.second;
  fh_.md_.fstat_.st_size += written;
  fh_.md_.fstat_.st_mtim.tv_sec = time_now;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  fh_.offset_ += written;
  // update this filedes in cache
  open_fds_->insert(filedes, fh_);
  // finished
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return written;
}
int KVFS::ChMod(const char *filename, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // check for existing file
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  md_.fstat_.st_uid = mode & S_ISUID;
  md_.fstat_.st_gid = mode & S_ISGID;
  md_.fstat_.st_mode |= mode;
  md_.fstat_.st_mtim.tv_sec = time_now;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->merge(key.pack(), md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Access(const char *filename, int how) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // check for existing file
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  if (!(md_.fstat_.st_mode & how)) {
    errorno_ = -EACCES;
    return errorno_;
  }

  return 0;
}
int KVFS::UTime(const char *filename, const struct utimbuf *times) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);

  // check for existing file
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  md_.fstat_.st_mtim.tv_sec = times->modtime;
  md_.fstat_.st_atim.tv_sec = times->actime;
  // store
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->put(key.pack(), md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Truncate(const char *filename, off_t length) {
  // check if length is less than then change the last block key,
  // if length is bigger, then create linked list of blocks necessary
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
  const std::string &key_str = key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  StoreResult sr = store_->get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "The filename arguments doesn't name an existing file or its an empty string");
  }
  kvfsMetaData md_;
  md_.parse(sr);
  kvfs_off_t new_number_of_blocks = length / KVFS_DEF_BLOCK_SIZE;
  new_number_of_blocks += (length % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  // compare to the file's number of blocks
  if (md_.fstat_.st_size > length) {
    // shrink it to length
    md_.fstat_.st_size = length;
    md_.last_block_key_.block_number_ = new_number_of_blocks;
  } else {
    // extend it, and put null bytes in the blocks
    md_.fstat_.st_size = length;
    // check how many to add
    kvfs_off_t blocks_to_add = new_number_of_blocks - md_.last_block_key_.block_number_;
    // get file's last block to build linked list
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    StoreResult sr = store_->get(md_.last_block_key_.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (!sr.isValid()) {
      return -1;
    }
    kvfsBlockValue bv_;
    bv_.parse(sr);
    kvfsBlockKey key_ = bv_.next_block_;
    // now extend it from here
    auto batch = store_->get_write_batch();
    for (int i = 0; i < blocks_to_add; ++i) {
      kvfsBlockValue tmp_ = kvfsBlockValue();
      tmp_.next_block_ = {key_.inode_, key_.block_number_ + 1};
      batch->put(key_.pack(), tmp_.pack());
      key_ = tmp_.next_block_;
    }
    batch->flush();
    batch.reset();
    md_.last_block_key_ = key_;
  }
  // update acccesstimes and modification times
  md_.fstat_.st_atim.tv_sec = time_now;
  md_.fstat_.st_mtim.tv_sec = time_now;
  // clear S_ISGUID and S_ISUID mode bits
  md_.fstat_.st_gid = 0;
  md_.fstat_.st_uid = 0;
  // store the new metadata
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->merge(key.pack(), md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Mknod(const char *filename, mode_t mode, dev_t dev) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.is_relative()) {
    // make it absolute
    std::filesystem::path prv_ = pwd_;
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
  std::pair<std::filesystem::path, std::pair<kvfsDirKey, kvfsMetaData>> resolved = RealPath(orig_);
  kvfsDirKey key = {resolved.second.second.fstat_.st_ino, std::filesystem::hash_value(resolved.first.filename())};
  const std::string &key_str = key.pack();
#if KVFS_THREAD_SAFE
  // lock
  mutex_->lock();
#endif
  if (store_->get(key_str).isValid()) {
    // exists return error
    errorno_ = -EEXIST;
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
  }
  kvfsMetaData md_ = kvfsMetaData(resolved.first.filename(),
                                  GetFreeInode(),
                                  mode | resolved.second.second.fstat_.st_mode,
                                  resolved.second.first);
  md_.fstat_.st_dev = dev;
  md_.fstat_.st_gid = mode;
  md_.fstat_.st_uid = mode;
  // update ctime
  md_.fstat_.st_ctim.tv_sec = time_now;
  // store
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->put(key_str, md_.pack());
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  // update the parent
  ++resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  bool status2 = store_->put(resolved.second.first.pack(), resolved.second.second.pack());
  return status;
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
//  inode_cache_.reset();
  open_fds_.reset();

  std::filesystem::remove_all(root_path);
}
uint32_t KVFS::GetFreeFD() {
  if (next_free_fd_ == KVFS_MAX_OPEN_FILES) {
    // error too many open files
    errorno_ = -ENFILE;
    throw FSError(FSErrorType::FS_ENFILE, "Too many files are currently open in the system.");
  } else {
    uint32_t fi = next_free_fd_;
    ++next_free_fd_;
    return fi;
  }
}
std::pair<ssize_t, kvfs::kvfsBlockKey> KVFS::WriteBlocks(kvfsBlockKey blck_key_,
                                                         size_t blcks_to_write_,
                                                         const void *buffer,
                                                         size_t buffer_size_,
                                                         kvfs_off_t offset) {
  std::unique_ptr<Store::WriteBatch> batch = store_->get_write_batch();
  const void *idx = buffer;
  ssize_t written = 0;
  std::string key_str;
  std::string value_str;
//  auto start = std::chrono::high_resolution_clock::now();
  kvfsBlockValue *bv_ = new kvfsBlockValue();
  for (size_t i = 0; i < blcks_to_write_; ++i) {
//    std::cout << blck_key_.block_number_ << "malloc: " << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
//    start = std::chrono::high_resolution_clock::now();
    std::pair<ssize_t, const void *> pair = FillBlock(bv_, idx, buffer_size_, offset);
//    std::cout << blck_key_.block_number_<< "memcpy: " <<  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    if (pair.first > 0) {
      offset = 0;
    }
    // if buffer size is still big get another block
    if (buffer_size_ > KVFS_DEF_BLOCK_SIZE) {
      bv_->next_block_ = kvfsBlockKey();
      bv_->next_block_.inode_ = blck_key_.inode_;
      bv_->next_block_.block_number_ = blck_key_.block_number_ + 1;
    }
#ifdef KVFS_DEBUG
    std::cout << blck_key_.block_number_ << " " << std::string((char *)bv_->data) << std::endl;
#endif
//    start = std::chrono::high_resolution_clock::now();
    key_str = blck_key_.pack();
    value_str = bv_->pack();
//    std::cout << blck_key_.block_number_<< "memcpy: " <<  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
//    start = std::chrono::high_resolution_clock::now();
    batch->put(key_str, value_str);
//    std::cout << blck_key_.block_number_<< "Put it batch: " <<  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
//    store_->put(blck_key_.pack(), bv_->pack());
    buffer_size_ -= pair.first;
    written += pair.first;
    // update offset
    idx = pair.second;
    blck_key_ = bv_->next_block_;
  }
  // flush the write batch
  batch->flush();
  batch.reset();
  delete[](bv_);
//    std::cout << blcks_to_write_ << " wrote for: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
  return std::pair<ssize_t, kvfs::kvfsBlockKey>(written, blck_key_);
}
std::pair<ssize_t, const void *> KVFS::FillBlock(kvfsBlockValue *blck_,
                                                 const void *buffer,
                                                 size_t buffer_size_,
                                                 kvfs_off_t offset) {
  if (offset >= KVFS_DEF_BLOCK_SIZE) {
    return std::pair<ssize_t, const void *>(0, buffer);
  }
  size_t max_writtable_size = (KVFS_DEF_BLOCK_SIZE - offset);
  const void *idx = buffer;
  if (buffer_size_ < max_writtable_size) {
//    auto start = std::chrono::high_resolution_clock::now();
    idx = blck_->write_at(buffer, buffer_size_, offset);
//    std::cout << "block write_at call: " <<  std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    return std::pair<ssize_t, const void *>(buffer_size_, idx);
  } else {
    idx = blck_->write_at(buffer, max_writtable_size, offset);
    return std::pair<ssize_t, const void *>(max_writtable_size, idx);
  }
}
ssize_t KVFS::ReadBlocks(kvfsBlockKey blck_key_,
                         size_t blcks_to_read_,
                         size_t buffer_size_,
                         off_t offset,
                         void *buffer) {
  void *idx = buffer;
  ssize_t read = 0;
  kvfsBlockValue bv_;
  for (size_t i = 0; i < blcks_to_read_; ++i) {
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    StoreResult sr = store_->get(blck_key_.pack());
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (sr.isValid()) {
      bv_.parse(sr);
#ifdef KVFS_DEBUG
      std::cout << blck_key_.block_number_ <<" " << std::string((char *)bv_.data) << std::endl;
#endif
      std::pair<ssize_t, void *> pair = ReadBlock(&bv_, idx, buffer_size_, offset);
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
  size_t max_readable_size = static_cast<size_t>(blck_->size_ - offset);
  void *idx = buffer;
  if (size < max_readable_size) {
    idx = blck_->read_at(buffer, size, offset);
    return std::pair<ssize_t, void *>(size, idx);
  } else {
    idx = blck_->read_at(buffer, max_readable_size, offset);
    return std::pair<ssize_t, void *>(max_readable_size, idx);
  }
}
std::pair<std::filesystem::path,
          std::pair<kvfs::kvfsDirKey, kvfs::kvfsMetaData>> KVFS::RealPath(const std::filesystem::path &input) {
  int symlink_loops = 0;
  std::filesystem::path real_path = input;
  for (;;) {
    auto pair = ResolvePath(real_path);
    auto second_pair = ResolvePath(pair.first);
    if (second_pair.first == pair.first) {
      // it is the real path
      return pair;
    } else {
      ++symlink_loops;
      if (symlink_loops > KVFS_LINK_MAX) {
        errorno_ = -ELOOP;
        throw FSError(FSErrorType::FS_ELOOP, "A loop exists in symbolic links encountered during resolution of "
                                             "the path");
      }
      real_path = second_pair.first;
    }
  }
}
int KVFS::UnMount() {
  kvfsDirKey root_key = {0, std::filesystem::hash_value("/")};
  store_->put("superblock", super_block_.pack());
  return 0;
}

}  // namespace kvfs