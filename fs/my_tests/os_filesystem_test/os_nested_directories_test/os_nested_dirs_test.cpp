#include <dirent.h>
#include <chrono>
#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <zconf.h>
/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * licemse that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_ms_test.cpp
 */

int main() {
  int flags = O_CREAT;
  flags |= O_WRONLY;
  mode_t mode = geteuid();
  std::string dirname;
  const char *name;
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "/tmp/myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    mkdir(name, mode);
  }
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "Created 10000 directories under /tmp/ for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  auto stream = opendir("/tmp/");
  start = std::chrono::high_resolution_clock::now();
  int count = 0;
  while (true) {
    auto entry = readdir(stream);
    ++count;
    if (!entry) {
      break;
    }
//    std::cout << "Found: " << entry->d_name << std::endl;
  }
  closedir(stream);
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read " << count << " directories for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  std::cout << "Changing directory to myfiles0 \n";
  chdir("/tmp/myfiles0/");
  std::cout << "Current directory name is: " << getcwd(nullptr, 0) << "\n";

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    mkdir(name, mode);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Created 10000 directories under myfiles0 for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  stream = opendir("/tmp/myfiles0/");
  start = std::chrono::high_resolution_clock::now();
  count = 0;
  while (true) {
    auto entry = readdir(stream);
    ++count;
    if (!entry) {
      break;
    }
//    std::cout << "Found: " << entry->d_name << std::endl;
  }
  closedir(stream);
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read " << count << " directories for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";

  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    rmdir(name);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Removed 10000 directories under myfiles0 for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";
  chdir("..");
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 10000; ++i) {
    dirname = "myfiles";
    dirname += std::to_string(i);
    name = dirname.c_str();
    rmdir(name);
  }
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Removed 10000 directories under /tmp for "
            << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count() << "ms\n";
  return 0;
}