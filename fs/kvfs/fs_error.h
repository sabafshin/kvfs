/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_error.h
 */

#ifndef KVFS_FS_ERROR_H
#define KVFS_FS_ERROR_H

#include <exception>
#include <string>
#include <errno.h>
#include <iostream>

namespace kvfs {
enum class FSErrorType : int8_t {
  FS_EINVAL = -EINVAL,
  FS_ERANGE = -ERANGE,
  FS_EACCES = -EACCES,
  FS_EBADF = -EBADF,
  FS_ENOTDIR = -ENOTDIR,
  FS_EINTR = -EINTR,
  FS_EIO = -EIO,
  FS_EEXIST = -EEXIST,
  FS_ENOENT = -ENOENT,
  FS_EMLINK = -EMLINK,
  FS_ENOSPC = -ENOSPC,
  FS_EROFS = -EROFS,
  FS_EPERM = -EPERM,
  FS_EXDEV = -EXDEV,
  FS_EBUSY = -EBUSY,
  FS_EISDIR = -EISDIR,
  FS_EFTYPE = -105,
  FS_ENAMETOOLONG = -ENAMETOOLONG,
  FS_ENFILE = -ENFILE,
  FS_ELOOP = -ELOOP,
  FS_EBADVALUESIZE = -106,
  FS_EBADFD = -EBADFD
};

class FSError : public std::exception {
 public:
  FSError(const FSErrorType &error_type, std::string message);

  const char *what() const noexcept override;
  const FSErrorType error_code() const noexcept;
 private:
  const FSErrorType &error_t;
  std::string msg_;
  std::string full_msg_;
};
}  // namespace kvfs
#endif //KVFS_FS_ERROR_H
