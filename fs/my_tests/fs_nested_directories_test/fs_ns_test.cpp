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
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1000; ++i) {
    std::string dirname = "myfiles";
    dirname += std::to_string(i);
    fs_->MkDir(dirname.c_str(), mode);
  }

  auto stream = fs_->OpenDir("/");
  while (true) {
    auto entry = fs_->ReadDir(stream);
    if (!entry) {
      break;
    }
//    std::cout << "Found: " << entry->d_name << std::endl;
  }

  fs_->CloseDir(stream);

  auto finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ns\n";

  fs_->DestroyFS();
  fs_.reset();

  return 0;
}