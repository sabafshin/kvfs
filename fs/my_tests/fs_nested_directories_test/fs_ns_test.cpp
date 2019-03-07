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

  auto res2 = fs_->MkDir("nestedFiles_1", mode);
  fs_->MkDir("nestedFiles_2", mode);
  fs_->MkDir("nestedFiles_3", mode);
  fs_->MkDir("nestedFiles_4", mode);
  fs_->MkDir("nestedFiles_5", mode);

  std::cout << res << std::endl;

  auto entry = fs_->ReadDir(stream);
  std::cout << entry->d_name << std::endl;

  entry = fs_->ReadDir(stream);
  std::cout << entry->d_name << std::endl;

  entry = fs_->ReadDir(stream);
  std::cout << entry->d_name << std::endl;

  entry = fs_->ReadDir(stream);
  std::cout << entry->d_name << std::endl;

  entry = fs_->ReadDir(stream);
  std::cout << entry->d_name << std::endl;

  auto final = fs_->CloseDir(stream);

  std::cout << final << std::endl;

  fs_->DestroyFS();
  fs_.reset();

  return 0;
}