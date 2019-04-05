/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * licemse that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_ms_test.cpp
 */

#include <kvfs/fs.h>
#include <kvfs/kvfs.h>

int main() {

  std::unique_ptr<FS> fs_ = std::make_unique<kvfs::KVFS>();
  int flags = O_CREAT;
  flags |= O_WRONLY;
  mode_t mode = geteuid();
  std::string dirname;
  const char *name;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    fs_->MkDir(name, mode);
  }
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "Created 10000 directories under root (/) for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  auto stream = fs_->OpenDir("/");
  start = std::chrono::high_resolution_clock::now();
  int count = 0;
  while (true) {
    auto entry = fs_->ReadDir(stream);
    ++count;
    if (!entry) {
      break;
    }
//    std::cout << "Found: " << entry->d_name << std::endl;
  }
  fs_->CloseDir(stream);
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read " << count << " directories for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  std::cout << "Changing directory to myfiles0 \n";
  fs_->ChDir("myfiles0");
  std::cout << "Current directory name is: " << fs_->GetCurrentDirName() << "\n";

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    fs_->MkDir(name, mode);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Created 10000 directories under myfiles0 for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  stream = fs_->OpenDir("/myfiles0");
  start = std::chrono::high_resolution_clock::now();
  count = 0;
  while (true) {
    auto entry = fs_->ReadDir(stream);
    ++count;
    if (!entry) {
      break;
    }
//    std::cout << "Found: " << entry->d_name << std::endl;
  }
  fs_->CloseDir(stream);
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read " << count << " directories for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    fs_->RmDir(name);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Removed 10000 directories under myfiles0 for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";
  fs_->ChDir("..");
  std::cout << "Current directory name is: " << fs_->GetCurrentDirName() << "\n";
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    fs_->RmDir(name);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Removed 10000 directories under /tmp for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";
  fs_->DestroyFS();
  fs_.reset();
  return 0;
}