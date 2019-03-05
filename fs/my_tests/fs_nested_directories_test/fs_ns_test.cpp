/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_ns_test.cpp
 */

#include <kvfs/fs.h>
#include <kvfs/kvfs.h>

int main() {

  std::unique_ptr<FS> fs_ = std::make_unique<kvfs::KVFS>();
  int flags = O_CREAT;
  flags |= O_WRONLY;
  mode_t mode = geteuid();

  auto dirname = "myfiles";
  auto res = fs_->MkDir(dirname, mode);
  auto stream = fs_->OpenDir("myfiles");

  auto res2 = fs_->MkDir("nestedFiles", mode);

  auto final = fs_->CloseDir(stream);

  std::cout << res << std::endl;
  std::cout << stream->file_descriptor_ << std::endl;
  std::cout << final << std::endl;

  fs_->DestroyFS();
  fs_.reset();

  // destroy db
//  system("rm -r /tmp/db/");

  return 0;
}