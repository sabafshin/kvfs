/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   os_rw_test.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <fcntl.h>
#include <algorithm>
#include <vector>
#include <random>

#define MAXREAD 1024*1024 // We don't read more than 1MB in one call

std::chrono::time_point t_start = std::chrono::high_resolution_clock::now();
// Simple timing
std::chrono::time_point t_end = std::chrono::high_resolution_clock::now();
long total_duration;
// Simple timing
void starttime() {
  t_start = std::chrono::high_resolution_clock::now();
}
void gettime_r(const char *desc, int repeat) {
  t_end = std::chrono::high_resolution_clock::now();
  long timedif = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
  total_duration += timedif;
  printf("%s time: %ldms\n", desc, timedif);
}
int main(int argc, char **argv) {
  // Setting some defaults
  int64_t blocksize = 4096;
  int block_count = 10;
  int file_count = 10;
  float loop = 1.0f;
  int rvalue;
  int debug = 0;
  while ((rvalue = getopt(argc, argv, "h--s:c:n:d")) != -1)
    switch (rvalue) {
      default:
        printf("Usage: %s [-s blocksize] [-c blockcount] [-n filecount] [-l loopcount (float)] [-d]\n",
               argv[0]);
        exit(0);
      case 's':sscanf(optarg, "%ld", &blocksize);
        if (blocksize < 0 || blocksize > 1e12) { // 0 .. 1 terabyte
          fprintf(stderr, "Blocksize of %ld bytes is out of bounds, resetting to 4096\n", blocksize);
          blocksize = 4096;
        }
        break;
      case 'c':sscanf(optarg, "%d", &block_count);
        break;
      case 'n':sscanf(optarg, "%d", &file_count);
        if (file_count < 0 || file_count > 1e9) { // 0 .. 1 billion
          fprintf(stderr, "Count out of bounds, resetting to 10\n", file_count);
          file_count = 10;
        }
        break;
    }
  int file_size = blocksize * block_count;
  printf("Creating %d files of size %d B", file_count, file_size);
  printf(", total data %.2f MB\n", (double) (file_size * file_count) / (1024.0 * 1024.0));

  int flags = O_CREAT;
  flags |= O_RDWR | O_APPEND | S_IFREG;
  mode_t mode = geteuid();
  auto *data = (unsigned char *) malloc(file_size);
  char filename[1024];
  for (int i = 0; i < MAXREAD && i < blocksize; ++i)
    data[i] = i % 256;
  printf("Writting\n");
  int64_t left;
  std::vector<int> vec;
  vec.reserve(file_count);
  for (int i = 0; i < file_count; ++i) {
    vec.push_back(i);
  }
  std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  for (const int &i : vec) {
    sprintf(filename, "rand%d", i);
    int f = open(filename, flags, mode);
    starttime();
    write(f, data, file_size);
    printf("File %s ", filename);
    gettime_r("read", file_count);
    close(f);
  }
  printf("Total taken time to write files: %ldms \n", total_duration);
  printf("Average file write time: %ldms\n", total_duration / file_count);
  printf("Reading\n");
  total_duration = 0;
  std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  for (const int &index : vec) {
    sprintf(filename, "rand%d", index);
    int f = open(filename, flags, mode);
    starttime();
    read(f, data, file_size);
    printf("File %s ", filename);
    gettime_r("read", file_count);
    close(f);
  }

  printf("Total taken time to read files: %ldms \n", total_duration);
  printf("Average files read time: %ldms\n", total_duration / file_count);
  printf("Removing\n");
  for (const int &i : vec) {
    sprintf(filename, "rand%d", i);
    remove(filename);
  }
  free(data);
}