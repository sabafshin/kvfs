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
    store_(std::make_shared<kvfsRocksDBStore>(mount_path)),
#endif
#if KVFS_HAVE_LEVELDB
      store_(std::make_shared<kvfsLevelDBStore>(mount_path)),
#endif
//      inode_cache_(std::make_unique<InodeCache>(KVFS_MAX_OPEN_FILES, store_)),
      open_fds_(std::make_unique<OpenFilesCache>(KVFS_MAX_OPEN_FILES)),
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
    store_(std::make_shared<kvfsRocksDBStore>(root_path)),
#endif
#if KVFS_HAVE_LEVELDB
      store_(std::make_shared<kvfsLevelDBStore>(root_path)),
#endif
//      inode_cache_(std::make_unique<InodeCache>(KVFS_MAX_OPEN_FILES, store_)),
      open_fds_(std::make_unique<OpenFilesCache>(KVFS_MAX_OPEN_FILES)),
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
  }
  std::string str = pwd_.string();
  if (str.size() > size) {
    errorno_ = -ERANGE;
    std::string msg = "The size argument is less than the length of the working directory name.\n"
                      "You need to allocate a bigger array and try again.";
    throw FSError(FSErrorType::FS_ERANGE, msg);
  }

  if (buffer != nullptr) {
    memcpy(buffer, str.data(), str.size());
    return buffer;
  }

  // unspecified behaviour
  return buffer;
}

std::string kvfs::KVFS::GetCurrentDirName() {
  return pwd_;
}

int kvfs::KVFS::ChDir(const char *path) {
  std::filesystem::path orig_ = std::filesystem::path(path);
  if (orig_ == "..") {
    pwd_ = pwd_.parent_path();
    return 0;
  }
  if (orig_ == ".") {
    return 0;
  }
  CheckNameLength(orig_);

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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }

  // retrieve the filename from store, if it doesn't exist then error
  kvfsInodeKey key_ = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key_.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (sr.isValid()) {
    // found the file, check if it is a directory
    kvfsInodeValue md_;
    md_.parse(sr);
    if (S_ISLNK(md_.fstat_.st_mode)) {
      // it is symbolic link get the symlink contents
      const std::filesystem::path &sym_link_contents_path = GetSymLinkContentsPath(md_);
      cwd_name_ = sym_link_contents_path.filename();
      pwd_ = sym_link_contents_path;
      return 0;
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
    cwd_name_ = orig_.filename();
    pwd_ = orig_;

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
  CheckNameLength(orig_);
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();

#if KVFS_THREAD_SAFE
  // lock
    mutex_->lock();
#endif
  kvfs_file_hash_t hash = 0;

  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  // if it is root, then inode number is 0
  kvfs_file_inode_t inode_number = (orig_ == "/") ? 0 : resolved.second.second.fstat_.st_ino;
  kvfsInodeKey key = {inode_number, hash};
  std::string key_str = key.pack();
  KVStoreResult sr = store_->Get(key_str);
  if (sr.isValid()) {
    kvfsInodeValue md_;
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
    open_fds_->Insert(fd_, fh_);

#if KVFS_THREAD_SAFE
    //unlock
        mutex_->unlock();
#endif
    //success
    kvfsDIR *result = new kvfsDIR();
    result->file_descriptor_ = fd_;
    result->ptr_ = store_->GetIterator();
    kvfsInodeKey seek_key = {md_.fstat_.st_ino, 0};
    key_str = seek_key.pack();
    result->ptr_->Seek(key_str);
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
  CheckNameLength(orig_);
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

  // Attempt to resolve the real path from given path
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(orig_.parent_path());
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  // now the path components all exist and resolved
  if ((flags & O_ACCMODE) == O_RDONLY || (flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
  } else {
    // undefined access mode flags
    return -EINVAL;
  }
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
  if (flags & O_CREAT) {
    // If set, the file will be created if it doesn’t already exist.
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    // ensure sr is valid
    if (sr.isValid()) {
      if (flags & O_EXCL) {
        // If both O_CREAT and O_EXCL are set, then open fails if the specified file already exists.
        // This is guaranteed to never clobber an existing file.
        errorno_ = -EEXIST;
        throw FSError(FSErrorType::FS_EEXIST, "Flags O_CREAT and O_EXCL are set but the file already exists");
      }
      kvfsInodeValue md_ = kvfsInodeValue();
      md_.parse(sr);
      kvfsFileHandle fh_ = kvfsFileHandle(key, md_, flags);
      uint32_t fd_ = GetFreeFD();
      // add it to open_fds
      open_fds_->Insert(fd_, fh_);
      return fd_;
    }

    // create a new file
    // check if there is room for more open files
    uint32_t fd_ = GetFreeFD();
    // file doesn't exist so create new one
    std::filesystem::path name = orig_.filename().string();
    kvfsInodeValue md_ = kvfsInodeValue(name, GetFreeInode(), mode, key);

    // generate a file descriptor
    kvfsFileHandle fh_ = kvfsFileHandle(key, md_, flags);


    // insert into store
    value_str = md_.pack();
#if KVFS_THREAD_SAFE
    // Acquire lock
          mutex_->lock();
#endif
    store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    // release lock
    mutex_->unlock();
#endif
    // write it back if flag O_SYNC
    if (flags & O_SYNC) {
      store_->Sync();
    }
    // add it to open_fds
    open_fds_->Insert(fd_, fh_);

    // update the parent
    ++resolved.second.second.fstat_.st_nlink;
    resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
    key_str = resolved.second.first.pack();
    value_str = resolved.second.second.pack();
#if KVFS_THREAD_SAFE
    // Acquire lock
    mutex_->lock();
#endif
    store_->Merge(key_str, value_str);
#if KVFS_THREAD_SAFE
    // release lock
    mutex_->unlock();
#endif
    return fd_;
  }

  // flag is not O_CREAT so open existing file
#if KVFS_THREAD_SAFE
  // Acquire lock
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  // release lock
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -EIO;
    return errorno_;
  }
  kvfsInodeValue md_ = kvfsInodeValue();
  md_.parse(sr);
  kvfsFileHandle fh_ = kvfsFileHandle(key, md_, flags);
  uint32_t fd_ = GetFreeFD();
  // add it to open_fds
  open_fds_->Insert(fd_, fh_);
  return fd_;
}

void kvfs::KVFS::FSInit() {
  if (store_ != nullptr) {
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    const KVStoreResult &sb = store_->Get("superblock");
    if (sb.isValid()) {
      super_block_.parse(sb);
      super_block_.fs_number_of_mounts_++;
      super_block_.fs_last_mount_time_ = time_now;
    } else {
      super_block_.fs_creation_time_ = time_now;
      super_block_.fs_last_mount_time_ = time_now;
      super_block_.fs_number_of_mounts_ = 1;
      super_block_.total_inode_count_ = 2;
      super_block_.next_free_inode_ = 2;
      std::string value_str = super_block_.pack();
      bool status = store_->Put("superblock", value_str);
      if (!status) {
        throw FSError(FSErrorType::FS_EIO, "Failed to put superblock in store");
      }
    }
    kvfsInodeKey root_key = {0, std::filesystem::hash_value("/")};
    std::string key_str = root_key.pack();
    std::string value_str;
    if (!store_->Get(root_key.pack()).isValid()) {
      mode_t mode = geteuid() | getegid() | S_IFDIR;
      kvfsInodeValue root_md = kvfsInodeValue("/", 1, mode, root_key);
      value_str = root_md.pack();
      store_->Put(key_str, value_str);
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
          std::pair<kvfsInodeKey, kvfsInodeValue>> kvfs::KVFS::ResolvePath(const std::filesystem::path &input) {
  std::filesystem::path output;
  kvfs_file_inode_t inode = 0;
  kvfsInodeKey prv_k;
  kvfsInodeValue prv_v;
  kvfsInodeKey parent_key_;
  kvfsInodeValue parent_md_;
  std::string key_str;
  kvfsInodeValue md_;
  // Assume the path is absolute
  for (const std::filesystem::path &e : input) {
    if (e == ".") {
      continue;
    }
    if (e == "..") {
      output = output.parent_path();
      if (input.filename() == "..") {
        parent_key_ = prv_k;
        parent_md_ = prv_v;
        break;
      }
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
      kvfsInodeKey key = {inode, std::filesystem::hash_value(e)};
      key_str = key.pack();
      KVStoreResult sr = store_->Get(key_str);
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
          prv_k = parent_key_;
          parent_key_ = md_.real_key_;
        } else {
          prv_k = parent_key_;
          parent_key_ = key;
        }
        // update inode to this file's inode
        inode = md_.fstat_.st_ino;
        prv_v = parent_md_;
        parent_md_ = md_;
      } else {
        // the path component must exists
        errorno_ = -ENONET;
        std::string msg = e.string() + " No such file or directory found under inode: #" + std::to_string(inode);
        throw FSError(FSErrorType::FS_ENOENT, msg);
      }
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
    }
    // append normally
    output.append(e.string());
  }
  return std::pair(output, std::pair(parent_key_, parent_md_));
}
std::filesystem::path kvfs::KVFS::GetSymLinkContentsPath(const kvfs::kvfsInodeValue &data) {
  std::filesystem::path output;
  std::list<std::filesystem::path> path_list;
  kvfsBlockKey blck_key = {data.fstat_.st_ino, 0};
  errorno_ = 0;
  // read contents of the symlink block and convert it to a path
  void *buffer = malloc(static_cast<size_t>(data.fstat_.st_size));
  size_t blcks_to_read = data.fstat_.st_size / KVFS_DEF_BLOCK_SIZE
      + ((data.fstat_.st_size % KVFS_DEF_BLOCK_SIZE) ? 1 : 0);
  ssize_t read = ReadBlocks(blck_key, blcks_to_read, static_cast<size_t>(data.fstat_.st_size), buffer);
  output = (char *) buffer;
  free(buffer);
  return output;
}
bool kvfs::KVFS::FreeUpInodeNumber(const kvfs_file_inode_t &inode) {
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  std::string key_str;
  std::string value_str;

  uint64_t free_inodes_key_number = super_block_.freed_inodes_count_ / 512;
  free_inodes_key_number += (super_block_.freed_inodes_count_ % 512) ? 1 : 0;
  kvfsFreedInodesKey fi_key = {"freeinodes", free_inodes_key_number};
  key_str = fi_key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (sr.isValid()) {
    kvfsFreedInodesValue fi_v;
    fi_v.parse(sr);
    ++fi_v.count_;
    fi_v.inodes[super_block_.freed_inodes_count_ % 512] = inode;
    value_str = fi_v.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    ++super_block_.freed_inodes_count_;
    --super_block_.total_inode_count_;
    return status;
  } else {
    kvfsFreedInodesValue fi_v;
    ++fi_v.count_;
    fi_v.inodes[super_block_.freed_inodes_count_ % 512] = inode;
    value_str = fi_v.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    ++super_block_.freed_inodes_count_;
    --super_block_.total_inode_count_;
    return status;
  }
}
ssize_t kvfs::KVFS::Read(int filedes, void *buffer, size_t size) {
  // read from offset in the file descriptor
  // if offset is at eof then return 0 else try to pread with filedes offset
  kvfsFileHandle fh_;
  bool status = open_fds_->Find(filedes, fh_);
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The file descriptor doesn't name a opened file, invalid fd");
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
    open_fds_->Insert(filedes, fh_);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return 0;
  } else {
    // try to read the file at the file descriptor position
    // then modify filedes offset by amount read
    ssize_t read = PRead(filedes, buffer, size, fh_.offset_);
    fh_.offset_ += read;
    open_fds_->Insert(filedes, fh_);
    return read;
  }
}
ssize_t kvfs::KVFS::Write(int filedes, const void *buffer, size_t size) {
  kvfsFileHandle fh_;
  bool status = open_fds_->Find(filedes, fh_);
  ssize_t written = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The file descriptor doesn't name a opened file, invalid fd");
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
    std::string key_str = blck_key_.pack();
    std::string value_str;
    KVStoreResult sr = store_->Get(key_str);
    if (sr.isValid()) {
      kvfsBlockValue bv_;
      bv_.parse(sr);
      auto pair = FillBlock(&bv_, buffer, size, blck_offset);
      written += pair.first;
      idx = pair.second;
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      value_str = bv_.pack();
      store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
      blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
    }

    size_t size_left = size - written;
    size_t blocks_to_write_ = (size_left) / KVFS_DEF_BLOCK_SIZE;
    blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
    ssize_t result =
        WriteBlocks(blck_key_, blocks_to_write_, idx, size_left);
    written += result;

    // update stats
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    fh_.md_.fstat_.st_size += written;
    fh_.offset_ = fh_.md_.fstat_.st_size;
    if ((fh_.flags_ & O_NOATIME) == 0)
      fh_.md_.fstat_.st_mtim.tv_sec = time_now;
    // update this filedes in cache
    open_fds_->Insert(filedes, fh_);
    // finished
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return written;
  } else {
    // call to PWrite to write from offset
    return PWrite(filedes, buffer, size, fh_.offset_);
  }
}
kvfs_file_inode_t kvfs::KVFS::GetFreeInode() {
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  std::string key_str;
  std::string value_str;
  kvfs_file_inode_t result;
  if (super_block_.freed_inodes_count_ > 0) {
    uint64_t free_inodes_key_number = super_block_.freed_inodes_count_ / 512;
    kvfsFreedInodesKey fi_key = {"freeinodes", free_inodes_key_number};
    key_str = fi_key.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (sr.isValid()) {
      kvfsFreedInodesValue fi_v;
      fi_v.parse(sr);
      result = fi_v.inodes[fi_v.count_];
      --fi_v.count_;
      value_str = fi_v.pack();
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      bool status = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
      mutex_->unlock();
#endif
      --super_block_.freed_inodes_count_;
      ++super_block_.total_inode_count_;
      return result;
    }
  }
  result = super_block_.next_free_inode_;
  ++super_block_.next_free_inode_;
  ++super_block_.total_inode_count_;
  return result;
}
int KVFS::Close(int filedes) {
  // check filedes exists
  try {
    kvfsFileHandle fh_;
#if KVFS_THREAD_SAFE
    mutex_->lock(); // lock
#endif
    bool status = open_fds_->Find(filedes, fh_);
    if (!status) {
      errorno_ = -EBADFD;
      throw FSError(FSErrorType::FS_EBADFD, "The given file des does not match any open files");
    }
    // Close just update the store anyways, and its upto the caller
    //  to correctly call close on deleted files.

    std::string key_str = fh_.key_.pack();
    std::string value_str = fh_.md_.pack();
    store_->Merge(key_str, value_str);
    // release it from open_fds
    open_fds_->Evict(filedes);
    FreeUpFD(filedes);
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
  if (!open_fds_->Find(dirstream->file_descriptor_, fh_)) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The dirp argument does not refer to an open directory stream.");
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
      if (dirstream->ptr_->key().size() == sizeof(kvfsInodeKey)
          && dirstream->ptr_->value().asString().size() == sizeof(kvfsInodeValue)) {
        kvfsInodeKey key;
        key.parse(dirstream->ptr_->key());
        kvfsInodeValue md_;
        md_.parse(dirstream->ptr_->value());

        if (key.inode_ == fh_.md_.dirent_.d_ino) {
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

  if (!open_fds_->Find(dirstream->file_descriptor_, fh_)) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The dirp argument does not refer to an open directory stream.");
  }
  try {
    // CloseDir update the file in store, and delete the dirstream
    std::string key_str = fh_.key_.pack();
    std::string value_str = fh_.md_.pack();
    // file is new or updated
    store_->Merge(key_str, value_str);
    // release it from open_fds
    open_fds_->Evict(dirstream->file_descriptor_);
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

  std::string key_str;
  std::string value_str;

  std::filesystem::path orig_old = std::filesystem::path(oldname);
  std::filesystem::path orig_new = std::filesystem::path(oldname);
  CheckNameLength(orig_old);
  CheckNameLength(orig_new);

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
  std::filesystem::path input = orig_old.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved_old =
      RealPath(input);
  resolved_old.first.append(orig_old.filename().string());
  orig_old = resolved_old.first.lexically_normal();

  input = orig_new.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved_new =
      RealPath(input);
  resolved_new.first.append(orig_new.filename().string());
  orig_new = resolved_new.first.lexically_normal();

  // create a new link (directory entry) for the existing file

  // check if old name exists

  kvfs_file_hash_t old_hash;
  if (orig_old != "/") {
    old_hash = std::filesystem::hash_value(orig_old.filename());
  } else {
    old_hash = std::filesystem::hash_value(orig_old);
  }

  kvfsInodeKey
      old_key = {resolved_old.second.second.fstat_.st_ino, old_hash};
  key_str = old_key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT, "A component of either path prefix does not exist; "
                                          "the file named by path1 does not exist; or path1 or path2 points to an empty string.");
  }
  kvfsInodeValue old_md_;
  old_md_.parse(sr);
  kvfs_file_hash_t new_hash;
  if (orig_new != "/") {
    new_hash = std::filesystem::hash_value(orig_new.filename());
  } else {
    new_hash = std::filesystem::hash_value(orig_new);
  }
  kvfsInodeKey
      new_key = {resolved_new.second.second.fstat_.st_ino, new_hash};
  // set the inode of this new link to the inode of the original file
  kvfsInodeValue new_md_ = kvfsInodeValue((orig_new == "/" ? orig_new : orig_new.filename()),
                                          old_md_.fstat_.st_ino,
                                          resolved_new.second.second.fstat_.st_mode,
                                          old_key);
  new_md_.dirent_.d_ino = GetFreeInode(); // setup a unique inode for this entry
  if (old_md_.real_key_ != old_key) {
    // check if original exists, if it doesn't exist then make the old one original
    key_str = old_md_.real_key_.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (!sr.isValid()) {
      old_md_.real_key_ = old_key;
      --old_md_.fstat_.st_nlink;
    } else {
      kvfsInodeValue original_md_;
      original_md_.parse(sr);
      ++original_md_.fstat_.st_nlink;
      value_str = original_md_.pack();
#if KVFS_THREAD_SAFE
      mutex_->lock();
#endif
      store_->Merge(key_str, value_str);
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
  key_str = old_key.pack();
  value_str = old_md_.pack();
  bool status1 = store_->Merge(key_str, value_str);
  key_str = new_key.pack();
  value_str = new_md_.pack();
  bool status2 = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1 & status2;
}
int KVFS::SymLink(const char *path1, const char *path2) {
  std::filesystem::path orig_path2 = std::filesystem::path(path1);
  CheckNameLength(orig_path2);
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

  std::filesystem::path input = orig_path2.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved_path_2 = RealPath(input);
  resolved_path_2.first.append(orig_path2.filename().string());
  orig_path2 = resolved_path_2.first.lexically_normal();
  // The symlink() function shall create a symbolic link called path2 that contains the string pointed to by path1
  // (path2 is the name of the symbolic link created, path1 is the string contained in the symbolic link).
  //The string pointed to by path1 shall be treated only as a string and shall not be validated as a pathname.
  kvfs_file_hash_t hash_path2;
  if (orig_path2 != "/") {
    hash_path2 = std::filesystem::hash_value(orig_path2.filename());
  } else {
    hash_path2 = std::filesystem::hash_value(orig_path2);
  }
  kvfsInodeKey slkey_ =
      {resolved_path_2.second.second.fstat_.st_ino, hash_path2};
  // check if the key names an existing file

  std::string key_str = slkey_.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (sr.isValid()) {
    // return error EEXISTS
    errorno_ = -EEXIST;
    throw FSError(FSErrorType::FS_EEXIST, "The path2 argument names an existing file.");
  }
  // create the symlink
  kvfsInodeValue slmd_ =
      kvfsInodeValue((orig_path2 == "/" ? orig_path2 : orig_path2.filename()),
                     GetFreeInode(),
                     S_IFLNK | resolved_path_2.second.second.fstat_.st_mode, slkey_);

  kvfsBlockKey sl_blck_key = kvfsBlockKey(slmd_.fstat_.st_ino, 0);
  size_t blcks_to_write = strlen(path1) / KVFS_DEF_BLOCK_SIZE;
  blcks_to_write += (strlen(path1) % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  ssize_t size = WriteBlocks(sl_blck_key, blcks_to_write, path1, strlen(path1));
  slmd_.fstat_.st_size = size;
  value_str = slmd_.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status1 = store_->Put(key_str, value_str);
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
  CheckNameLength(orig_);
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

  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>>
      resolved = RealPath(orig_.parent_path());
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};

  std::string key_str = key.pack();
  // check if the key names an existing file
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  // check if it is a symlink
  if (!S_ISLNK(md_.fstat_.st_mode)) {
    // not a symlink
    errorno_ = -ENOLINK;
    return errorno_;
  }
  kvfsBlockKey blck_key_ = kvfsBlockKey(md_.fstat_.st_ino, 0);
  ssize_t size_to_read = (size > md_.fstat_.st_size ? md_.fstat_.st_size : size);
  size_t blcks_to_read = size_to_read / KVFS_DEF_BLOCK_SIZE;
  blcks_to_read += (size_to_read % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  ssize_t pair = ReadBlocks(blck_key_, blcks_to_read, size, buffer);
  return pair;
}
int KVFS::UnLink(const char *filename) {
  // decrease file's link count by one, if it reaches zero then delete the file from store
  std::filesystem::path orig_ = std::filesystem::path(filename);
  if (orig_.filename() == "." || orig_.filename() == "..") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The path argument contains a last component that is dot.");
  }
  CheckNameLength(orig_);
  if (orig_ == "/") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "The path argument refers to the root directory,"
                                          " it is not possible to remove root directory.");
  }
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

  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(orig_.parent_path());
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();

  kvfs_file_hash_t hash = std::filesystem::hash_value(orig_.filename());
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};

  std::string key_str = key.pack();
  std::string value_str;

#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENOENT;
    throw FSError(FSErrorType::FS_ENOENT, "No such file or directory!");
  }
  // found the file
  kvfsInodeValue md_;
  md_.parse(sr);
  if (S_ISLNK(md_.fstat_.st_mode)) {
    // just delete it
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->Delete(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    status &= FreeUpInodeNumber(md_.fstat_.st_ino);
    return status;
  }
  // decrease link count
  --md_.fstat_.st_nlink;
  if (md_.fstat_.st_nlink <= 0) {
    // remove it from store
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status = store_->Delete(key_str);
    status &= FreeUpInodeNumber(md_.dirent_.d_ino);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    // update the parent
    --resolved.second.second.fstat_.st_nlink;
    resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
    key_str = resolved.second.first.pack();
    value_str = resolved.second.second.pack();

#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    bool status2 = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    return status & status2;
  }
  md_.fstat_.st_mtim.tv_sec = time_now;
  // update it in store
  value_str = md_.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status1 = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif

  // update the parent
  --resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  key_str = resolved.second.first.pack();
  value_str = resolved.second.second.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status2 = store_->Put(key_str, value_str);
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

  std::string key_str;
  std::string value_str;

  std::filesystem::path oldname_orig = std::filesystem::path(oldname);
  std::filesystem::path newname_orig = std::filesystem::path(newname);
  CheckNameLength(oldname_orig);
  CheckNameLength(newname_orig);
  if (oldname_orig.lexically_normal() == "/") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "Root directory cannot be renamed");
  }
  if (newname_orig.lexically_normal() == "/") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "Root directory cannot be renamed");
  }
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
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>>
      oldname_resolved = RealPath(oldname_orig.parent_path());
  oldname_resolved.first.append(oldname_orig.filename().string());
  oldname_orig = oldname_resolved.first.lexically_normal();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>>
      newname_resolved = RealPath(newname_orig.parent_path());
  newname_resolved.first.append(newname_orig.filename().string());
  newname_orig = newname_resolved.first.lexically_normal();
  // check if oldname exists and newname doesn't exist
  kvfs_file_hash_t old_hash = std::filesystem::hash_value(oldname_orig.filename());
  kvfs_file_hash_t new_hash = std::filesystem::hash_value(newname_orig.filename());
  kvfsInodeKey old_key =
      {oldname_resolved.second.second.fstat_.st_ino, old_hash};
  kvfsInodeKey new_key =
      {newname_resolved.second.second.fstat_.st_ino, new_hash};
  key_str = old_key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult old_sr = store_->Get(key_str);
  key_str = new_key.pack();
  bool new_exists = store_->Get(key_str).isValid();
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
  auto batch = store_->GetWriteBatch();
  // decrease link count on old name's parent
  oldname_resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  --oldname_resolved.second.second.fstat_.st_nlink;
  key_str = old_key.pack();
  value_str = oldname_resolved.second.second.pack();
  batch->Delete(key_str);
  key_str = oldname_resolved.second.first.pack();
  batch->Delete(key_str);
  batch->Put(key_str, value_str);
  // copy over old metadata under new key
  kvfsInodeValue new_md_;
  new_md_.parse(old_sr);
  std::string new_name = newname_resolved.first.filename();
  kvfs_dirent new_dirent{};
  new_dirent.d_ino = new_md_.dirent_.d_ino;
  new_name.copy(new_dirent.d_name, new_name.size(), 0);
  new_md_.dirent_ = new_dirent;
  key_str = new_key.pack();
  value_str = new_md_.pack();
  batch->Put(key_str, value_str);
  batch->Flush();
  batch.reset();
  return 0;
}
int KVFS::MkDir(const char *filename, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  if (orig_.lexically_normal() == "/") {
    errorno_ = -EINVAL;
    throw FSError(FSErrorType::FS_EINVAL, "Cannot create a directory with name (\"/\") !");
  }
  // Attemp to resolve the real path from given path
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(orig_.parent_path());
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();

  // now try to make dir at that parent directory
  // resolver does set current md and key to the parent of this file so use those and then update it in store.
  kvfs_file_hash_t hash = std::filesystem::hash_value(orig_.filename());
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  // lock
  mutex_->lock();
#endif
  if (store_->Get(key_str).isValid()) {
    // exists return error
    errorno_ = -EEXIST;
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
  }
  kvfsInodeValue md_ = kvfsInodeValue(orig_.filename(),
                                      GetFreeInode(),
                                      mode | resolved.second.second.fstat_.st_mode, key);

  // update meta data
  md_.fstat_.st_mode |= S_IFDIR;
  // set this file's group id to its parent's
  md_.fstat_.st_gid = resolved.second.second.fstat_.st_gid;
  // update ctime
  md_.fstat_.st_ctim.tv_sec = time_now;
  // update it in store
  value_str = md_.pack();
  bool status1 = store_->Put(key_str, value_str);
  // update the parent
  ++resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  key_str = resolved.second.first.pack();
  value_str = resolved.second.second.pack();
  bool status2 = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status1 & status2;
}
int KVFS::Stat(const char *filename, kvfs_stat *buf) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  // check for existing file
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  *buf = md_.fstat_;
  return 0;
}
off_t KVFS::LSeek(int filedes, off_t offset, int whence) {
  kvfsFileHandle fh_;
  bool status = open_fds_->Find(filedes, fh_);
  ssize_t read = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The file descriptor doesn't name a opened file, invalid fd");
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
  open_fds_->Evict(filedes);
  open_fds_->Insert(filedes, fh_);

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
  store_->Sync();
}
int KVFS::FSync(int filedes) {
  // currently only same as sync
  this->Sync();
  return 0;
}
ssize_t KVFS::PRead(int filedes, void *buffer, size_t size, off_t offset) {
  // only read the file from offset argument, doesn't modify filedes offset
  kvfsFileHandle fh_;
  bool status = open_fds_->Find(filedes, fh_);
  ssize_t read = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first
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
    open_fds_->Insert(filedes, fh_);
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
  std::string key_str = blck_key_.pack();
  std::string value_str;
  KVStoreResult sr = store_->Get(key_str);
  if (sr.isValid()) {
    kvfsBlockValue bv_;
    bv_.parse(sr);
    auto pair = ReadBlock(&bv_, buffer, size, blck_offset);
    read += pair.first;
    idx = pair.second;
    value_str = bv_.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
  }

  size_can_read -= read;
  size_t blocks_to_read_ = size_can_read / KVFS_DEF_BLOCK_SIZE;
  blocks_to_read_ += ((size_can_read % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
  read += ReadBlocks(blck_key_, blocks_to_read_, size_can_read, idx);
  // finished
  // update stats and return
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  if (!(fh_.flags_ & O_NOATIME)) {
    // update accesstimes on the file
    fh_.md_.fstat_.st_atim.tv_sec = time_now;
  }
  open_fds_->Insert(filedes, fh_);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return read;
}

ssize_t KVFS::PWrite(int filedes, const void *buffer, size_t size, off_t offset) {
  kvfsFileHandle fh_;
  bool status = open_fds_->Find(filedes, fh_);
  ssize_t written = 0;
  if (!status) {
    errorno_ = -EBADFD;
    throw FSError(FSErrorType::FS_EBADFD, "The file descriptor doesn't name a opened file, invalid fd");
  }
  // check flags first

  if (offset > fh_.md_.fstat_.st_size) {
    // offset exceeds file size
    errorno_ = -E2BIG;
    throw FSError(FSErrorType::FS_EINTR, "The offset argument exceeds the filedes's file size.");
  }

  // determine which block to write from, then
  // pass that to blocks writer.

  kvfsBlockKey blck_key_;
  blck_key_.inode_ = fh_.md_.fstat_.st_ino;
  blck_key_.block_number_ = offset / KVFS_DEF_BLOCK_SIZE;
  kvfs_off_t blck_offset = offset % KVFS_DEF_BLOCK_SIZE;
  const void *idx = buffer;
  std::string key_str = blck_key_.pack();
  std::string value_str;

  KVStoreResult sr = store_->Get(key_str);
  if (sr.isValid()) {
    kvfsBlockValue bv_;
    bv_.parse(sr);
    auto pair = FillBlock(&bv_, buffer, size, blck_offset);
    written += pair.first;
    idx = pair.second;
    value_str = bv_.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    blck_key_ = kvfsBlockKey(fh_.md_.fstat_.st_ino, blck_key_.block_number_ + 1);
  }

  size_t size_left = size - written;
  size_t blocks_to_write_ = (size_left) / KVFS_DEF_BLOCK_SIZE;
  blocks_to_write_ += ((size_left % KVFS_DEF_BLOCK_SIZE) > 0) ? 1 : 0;
  ssize_t result =
      WriteBlocks(blck_key_, blocks_to_write_, idx, size_left);
  written += result;

  // update stats
  if ((fh_.flags_ & O_NOATIME) == 0)
    fh_.md_.fstat_.st_mtim.tv_sec = time_now;
  fh_.offset_ += written;

  fh_.md_.fstat_.st_size = (fh_.offset_ >= fh_.md_.fstat_.st_size) ? fh_.offset_ + 1 : fh_.md_.fstat_.st_size;

#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  // update this filedes in cache
  open_fds_->Insert(filedes, fh_);
  // finished
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return written;
}
int KVFS::ChMod(const char *filename, mode_t mode) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
  if (orig_.lexically_normal() == "/") {
    errorno_ = -EACCES;
    throw FSError(FSErrorType::FS_EACCES, "Cannot change stats of root directory");
  }
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  // check for existing file
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  md_.fstat_.st_uid = mode & S_ISUID;
  md_.fstat_.st_gid = mode & S_ISGID;
  md_.fstat_.st_mode |= mode;
  md_.fstat_.st_mtim.tv_sec = time_now;
  value_str = md_.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->Merge(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Access(const char *filename, int how) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  // check for existing file
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  if (!(md_.fstat_.st_mode & how)) {
    errorno_ = -EACCES;
    return errorno_;
  }

  return 0;
}
int KVFS::UTime(const char *filename, const struct utimbuf *times) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }

  // check for existing file
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "A component of path does not name an existing file or path is an empty string.");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  md_.fstat_.st_mtim.tv_sec = times->modtime;
  md_.fstat_.st_atim.tv_sec = times->actime;
  value_str = md_.pack();
  // store
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Truncate(const char *filename, off_t length) {
  // check if length is less than then change the last block key,
  // if length is bigger, then create linked list of blocks necessary
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  std::filesystem::path input = orig_.parent_path();
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(input);
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();
  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }
  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  if (!sr.isValid()) {
    errorno_ = -ENONET;
    throw FSError(FSErrorType::FS_ENOENT,
                  "The filename arguments doesn't name an existing file or its an empty string");
  }
  kvfsInodeValue md_;
  md_.parse(sr);
  kvfs_off_t new_number_of_blocks = length / KVFS_DEF_BLOCK_SIZE;
  new_number_of_blocks += (length % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
  // compare to the file's number of blocks
  if (md_.fstat_.st_size > length) {
    // shrink it to length
    md_.fstat_.st_size = length;
  } else {
    kvfs_off_t blck_offset = md_.fstat_.st_size / KVFS_DEF_BLOCK_SIZE;
    blck_offset += (md_.fstat_.st_size % KVFS_DEF_BLOCK_SIZE) ? 1 : 0;
    // extend it, and put null bytes in the blocks
    md_.fstat_.st_size = length;
    // check how many to add
    kvfs_off_t blocks_to_add = new_number_of_blocks - blck_offset;
    // get file's last block to build linked list
    kvfsBlockKey last_block_key{};
    last_block_key.inode_ = md_.fstat_.st_ino;
    last_block_key.block_number_ = blck_offset;
    key_str = last_block_key.pack();
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (!sr.isValid()) {
      // error getting file's last block
      return -1;
    }
    kvfsBlockValue bv_;
    bv_.parse(sr);
    kvfsBlockKey block_key = last_block_key;
    block_key.block_number_ += 1;
    // now extend it from here
    auto batch = store_->GetWriteBatch();
    kvfsBlockValue tmp_ = kvfsBlockValue();
    for (int i = 0; i < blocks_to_add; ++i) {
      tmp_.next_block_ = block_key;
      tmp_.next_block_.block_number_ += 1;
      key_str = key.pack();
      value_str = tmp_.pack();
      batch->Put(key_str, value_str);
      block_key = tmp_.next_block_;
    }
    batch->Flush();
    batch.reset();
  }
  // update acccesstimes and modification times
  md_.fstat_.st_atim.tv_sec = time_now;
  md_.fstat_.st_mtim.tv_sec = time_now;
  // clear S_ISGUID and S_ISUID mode bits
  md_.fstat_.st_gid = 0;
  md_.fstat_.st_uid = 0;
  // store the new metadata
  key_str = key.pack();
  value_str = md_.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->Merge(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  return status;
}
int KVFS::Mknod(const char *filename, mode_t mode, dev_t dev) {
  std::filesystem::path orig_ = std::filesystem::path(filename);
  CheckNameLength(orig_);
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
  std::pair<std::filesystem::path, std::pair<kvfsInodeKey, kvfsInodeValue>> resolved = RealPath(orig_.parent_path());
  resolved.first.append(orig_.filename().string());
  orig_ = resolved.first.lexically_normal();

  kvfs_file_hash_t hash;
  if (orig_ != "/") {
    hash = std::filesystem::hash_value(orig_.filename());
  } else {
    hash = std::filesystem::hash_value(orig_);
  }

  kvfsInodeKey key = {resolved.second.second.fstat_.st_ino, hash};
  std::string key_str = key.pack();
  std::string value_str;
#if KVFS_THREAD_SAFE
  // lock
  mutex_->lock();
#endif
  if (store_->Get(key_str).isValid()) {
    // exists return error
    errorno_ = -EEXIST;
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
  }
  kvfsInodeValue md_ = kvfsInodeValue(orig_.filename(),
                                      GetFreeInode(),
                                      mode | resolved.second.second.fstat_.st_mode, key);
  md_.fstat_.st_dev = dev;
  md_.fstat_.st_gid = mode;
  md_.fstat_.st_uid = mode;
  // update ctime
  md_.fstat_.st_ctim.tv_sec = time_now;
  // store
  value_str = md_.pack();
#if KVFS_THREAD_SAFE
  mutex_->lock();
#endif
  bool status = store_->Put(key_str, value_str);
#if KVFS_THREAD_SAFE
  mutex_->unlock();
#endif
  // update the parent
  ++resolved.second.second.fstat_.st_nlink;
  resolved.second.second.fstat_.st_mtim.tv_sec = time_now;
  key_str = resolved.second.first.pack();
  value_str = resolved.second.second.pack();
  bool status2 = store_->Put(key_str, value_str);
  return status;
}
void KVFS::TuneFS() {
  store_->Compact();

  store_->Sync();
}
void KVFS::DestroyFS() {
  if (!store_->Destroy()) {
    throw FSError(FSErrorType::FS_EIO, "Failed to destroy store");
  }
  store_.reset();
  open_fds_.reset();
  std::filesystem::remove_all(root_path);
}
uint32_t KVFS::GetFreeFD() {
  uint32_t fi;
  if (!free_fds.empty()) {
    fi = free_fds.back();
    free_fds.pop_back();
    return fi;
  }
  if (next_free_fd_ == KVFS_MAX_OPEN_FILES) {
    // error too many open files
    errorno_ = -ENFILE;
    throw FSError(FSErrorType::FS_ENFILE, "Too many files are currently open in the system.");
  } else {
    fi = next_free_fd_;
    ++next_free_fd_;
    return fi;
  }
}
ssize_t KVFS::WriteBlocks(kvfsBlockKey blck_key_, size_t blcks_to_write_, const void *buffer, size_t buffer_size_) {
  std::unique_ptr<KVStore::WriteBatch> batch = store_->GetWriteBatch();
  const void *idx = buffer;
  ssize_t written = 0;
  std::string key_str;
  std::string value_str;
  kvfsBlockValue *bv_ = new kvfsBlockValue();
  for (size_t i = 0; i < blcks_to_write_; ++i) {
    std::pair<ssize_t, const void *> pair = FillBlock(bv_, idx, buffer_size_, 0);
    // if buffer size is still big get another block
    if (buffer_size_ > KVFS_DEF_BLOCK_SIZE) {
      bv_->next_block_ = kvfsBlockKey();
      bv_->next_block_.inode_ = blck_key_.inode_;
      bv_->next_block_.block_number_ = blck_key_.block_number_ + 1;
    }
#ifdef KVFS_DEBUG
    std::cout << blck_key_.block_number_ << " " << std::string((char *)bv_->data) << std::endl;
#endif
    key_str = blck_key_.pack();
    value_str = bv_->pack();
    batch->Put(key_str, value_str);
    buffer_size_ -= pair.first;
    written += pair.first;
    // update offset
    idx = pair.second;
    blck_key_ = bv_->next_block_;
  }
  // flush the write batch
  batch->Flush();
  batch.reset();
  delete (bv_);
  return written;
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
    idx = blck_->write_at(buffer, buffer_size_, offset);
    return std::pair<ssize_t, const void *>(buffer_size_, idx);
  } else {
    idx = blck_->write_at(buffer, max_writtable_size, offset);
    return std::pair<ssize_t, const void *>(max_writtable_size, idx);
  }
}
ssize_t KVFS::ReadBlocks(kvfsBlockKey blck_key_, size_t blcks_to_read_, size_t buffer_size_, void *buffer) {
  void *idx = buffer;
  ssize_t read = 0;
  kvfsBlockValue bv_;
  std::string key_str = blck_key_.pack();
  for (size_t i = 0; i < blcks_to_read_; ++i) {
#if KVFS_THREAD_SAFE
    mutex_->lock();
#endif
    KVStoreResult sr = store_->Get(key_str);
#if KVFS_THREAD_SAFE
    mutex_->unlock();
#endif
    if (sr.isValid()) {
      bv_.parse(sr);
#ifdef KVFS_DEBUG
      std::cout << blck_key_.block_number_ <<" " << std::string((char *)bv_.data) << std::endl;
#endif
      std::pair<ssize_t, void *> pair = ReadBlock(&bv_, idx, buffer_size_, 0);
      idx = pair.second;
      read += pair.first;
      blck_key_ = bv_.next_block_;
      buffer_size_ -= pair.first;
      key_str = blck_key_.pack();
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
          std::pair<kvfs::kvfsInodeKey, kvfs::kvfsInodeValue>> KVFS::RealPath(const std::filesystem::path &input) {
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
  std::string value_str = super_block_.pack();
  store_->Put("superblock", value_str);
  store_->Sync();
  return 0;
}
void KVFS::FreeUpFD(uint32_t filedes) {
  free_fds.push_back(filedes);
}

}  // namespace kvfs