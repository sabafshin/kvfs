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
enum class FSErrorType : uint8_t {
  FS_EINVAL,
  FS_ERANGE,
  FS_EACCES,
  FS_EBADF,
  FS_ENOTDIR,
  FS_EINTR,
  FS_EIO,
  FS_EEXIST,
  FS_ENOENT,
  FS_EMLINK,
  FS_ENOSPC,
  FS_EROFS,
  FS_EPERM,
  FS_EXDEV,
  FS_EBUSY,
  FS_EISDIR,
  FS_EFTYPE,
  FS_ENAMETOOLONG,
  FS_ENFILE,
  FS_ELOOP
};

class FSError : public std::exception {
 public:
  FSError(const FSErrorType &errorno, const std::string &message);

  const char *what() const noexcept override;

 private:
  FSErrorType error_t;
  std::string msg_;
  std::string full_msg_;
};
}  // namespace kvfs
#endif //KVFS_FS_ERROR_H
