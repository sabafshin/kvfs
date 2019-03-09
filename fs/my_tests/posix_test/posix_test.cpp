/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   posix_test.cpp
 */

#include <stdio.h>
#include <string>
#include <zconf.h>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include <cstring>

int main() {
  auto contents = "123456789abcdefghijklmnop";

  /*int fd = open("/tmp/something.txt", O_CREAT | O_RDWR | O_APPEND);

  auto written = write(fd, contents, 10);
  std::cout << written << std::endl;

  lseek(fd, 2, SEEK_SET);
  char b;
  for (int i = 0; i < 8; ++i) {
    auto is_read = read(fd, &b, 1);
    if (is_read < 0)
      break;
//    std::cout << is_read;
    std::cout << b;
  }

  remove("/tmp/something.txt");*/
  mode_t mode = geteuid();
  auto directory = opendir(get_current_dir_name());
  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < 1000; ++i) {
    std::string dirname = "myfiles";
    dirname += std::to_string(i);
    mkdir(dirname.c_str(), mode);
  }

  while (true) {
    auto dire = readdir(directory);
    if (!dire) {
      break;
    }
//    std::cout <<"Found: " << dire->d_name << std::endl;
  }

  closedir(directory);

  auto finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ns\n";

//  system("rm -r ./myfiles*");

/*  auto file = open("/tmp/something.txt", mode);
  write(file ,contents, strlen(contents));

  lseek(file, 0, SEEK_SET);

  void *buffer_r = std::malloc(strlen(contents));
  auto is_read = read(file,buffer_r, 25);
  close(file);

  auto finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << "ns\n";

  remove("/tmp/something.txt");*/

  return 0;
}