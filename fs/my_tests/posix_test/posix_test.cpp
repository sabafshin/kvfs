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
#include <chrono>
#include <cstring>
#include <fcntl.h>

void write_read_test() {
  int flags = O_CREAT;
  flags |= O_RDWR | O_APPEND;
  std::cout << "File opened with flags: " << flags << std::endl;
  mode_t mode = geteuid();
//  std::string file_name = "../..////./Hi.txt";
  std::string file_name = "Hi.txt";

  std::string data(100 * 1024 * 1024, 'x'); // 25 bytes
  size_t data_size = 100 * 1024 * 1024;

  int fd_ = open(file_name.c_str(), flags, mode);

  auto start = std::chrono::high_resolution_clock::now();
  int TOTAL_DATA = 1000000;

  const void *buffer_w = data.c_str();

  ssize_t total_data_size = write(fd_, buffer_w, data_size);

  /*for (int i = 0; i < TOTAL_DATA; i++) {

    ssize_t size = write(fd_, buffer_w, data_size);

    total_data_size += size;

  }*/

  std::cout << "Wrote : " << total_data_size << std::endl;
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms\n";
  total_data_size = 0;
  void *buffer_r = std::malloc(data_size);

  start = std::chrono::high_resolution_clock::now();
  total_data_size = pread(fd_, buffer_r, data_size, 0);
  /* for (int i = 0; i < TOTAL_DATA; ++i) {

     void *buffer_r = std::malloc(data_size);

     auto cur_read = pread(fd_, buffer_r, data_size, total_data_size);
     free(buffer_r);

     total_data_size += cur_read;
   }*/
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read : " << total_data_size << std::endl;
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms\n";
  fsync(fd_);
  close(fd_);
  remove(file_name.c_str());
}

int main() {
//  auto contents = "123456789abcdefghijklmnop";

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
  /*mode_t mode = geteuid();
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

  system("rm -r ./myfiles*");*/

/*  auto file = open("/tmp/something.txt", mode);
  write(file ,contents, strlen(contents));

  lseek(file, 0, SEEK_SET);

  void *buffer_r = std::malloc(strlen(contents));
  auto is_read = read(file,buffer_r, 25);
  close(file);

  auto finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() << "ns\n";

  remove("/tmp/something.txt");*/

  write_read_test();
  return 0;
}