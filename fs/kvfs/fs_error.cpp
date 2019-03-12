/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_error.cpp
 */

#include "fs_error.h"

namespace kvfs {

FSError::FSError
    (const FSErrorType &error_type_, const std::string &message)
    : error_t(error_type_), msg_(message) {
  full_msg_ = "[FS ERROR] : ";
  switch (error_t) {
    case FSErrorType::FS_EINVAL:full_msg_ += "(EINVAL) ";
      break;
    case FSErrorType::FS_ERANGE:full_msg_ += "(ERANGE) ";
      break;
    case FSErrorType::FS_EACCES:full_msg_ += "(EACCESS) ";
      break;
    case FSErrorType::FS_EBADF:full_msg_ += "(EBADF) ";
      break;
    case FSErrorType::FS_ENOTDIR:full_msg_ += "(ENOTDIR) ";
      break;
    case FSErrorType::FS_EINTR:full_msg_ += "(EINTR) ";
      break;
    case FSErrorType::FS_EIO:full_msg_ += "(EIO) ";
      break;
    case FSErrorType::FS_EEXIST:full_msg_ += "(EEXIST) ";
      break;
    case FSErrorType::FS_ENOENT:full_msg_ += "(ENOENT) ";
      break;
    case FSErrorType::FS_EMLINK:full_msg_ += "(EMLINK) ";
      break;
    case FSErrorType::FS_ENOSPC:full_msg_ += "(ENOSPC) ";
      break;
    case FSErrorType::FS_EROFS:full_msg_ += "(EROFS) ";
      break;
    case FSErrorType::FS_EPERM:full_msg_ += "(EPERM) ";
      break;
    case FSErrorType::FS_EXDEV:full_msg_ += "(EXDEV) ";
      break;
    case FSErrorType::FS_EBUSY:full_msg_ += "(EBUSY) ";
      break;
    case FSErrorType::FS_EISDIR:full_msg_ += "(EISDIR) ";
      break;
    case FSErrorType::FS_EFTYPE:full_msg_ += "(EFTYPE) ";
      break;
    case FSErrorType::FS_ENAMETOOLONG:full_msg_ += "(ENAMETOOLONG) ";
      break;
    case FSErrorType::FS_ENFILE:full_msg_ += "(ENFILE) ";
      break;
    case FSErrorType::FS_ELOOP:full_msg_ += "(ELOOP) ";
      break;
  }

  full_msg_ += "(" + msg_ + ")";

}
const char *FSError::what() const noexcept {
  return full_msg_.c_str();
}

}  // namespace kvfs
