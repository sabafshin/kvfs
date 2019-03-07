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
#include <sys/file.h>
#include <zconf.h>
#include <iostream>

int main() {
  auto contents = "123456789";

  int fd = open("/tmp/something.txt", O_CREAT | O_RDWR | O_APPEND);

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

  remove("/tmp/something.txt");
  /*mode_t  mode;
  auto file = fopen("/tmp/something.txt", "w");
  fputs(contents, file);

  freopen("/tmp/something.txt", "r", file);

  for (int i =0; i < 10; ++i){
    auto is_read = fgetc(file);
    std::cout << is_read;
    std::cout << " ";
  }

  fclose(file);
  remove("/tmp/something.txt");*/
  return 0;
}