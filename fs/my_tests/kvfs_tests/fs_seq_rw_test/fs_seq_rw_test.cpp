#include <random>

/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_random_rw_test.cpp
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <kvfs/fs.h>
#include <kvfs/kvfs.h>

#define MAXREAD 1024*1024 // We don't read more than 1MB in one call

std::chrono::time_point t_start = std::chrono::high_resolution_clock::now();
// Simple timing
std::chrono::time_point t_end = std::chrono::high_resolution_clock::now();
long total_duration;
std::vector<long> read_times;
std::vector<long> write_times;

// Simple timing
void starttime() {
  t_start = std::chrono::high_resolution_clock::now();
}
void gettime_r() {
  t_end = std::chrono::high_resolution_clock::now();
  long timedif = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
  read_times.push_back(timedif);
  total_duration += timedif;
}
void gettime_w() {
  t_end = std::chrono::high_resolution_clock::now();
  long timedif = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
  write_times.push_back(timedif);
  total_duration += timedif;
}

int main(int argc, char **argv) {
  // Setting some defaults
  int64_t blocksize = 4096;
  int block_count = 10;
  int file_count = 10;
  int rvalue;
  std::unique_ptr<FS> fs_ = std::make_unique<kvfs::KVFS>();

  while ((rvalue = getopt(argc, argv, "h--s:c:n:d")) != -1)
    switch (rvalue) {
      default:
        printf("Usage: %s [-s blocksize] [-c blockcount] [-n filecount] [-l loopcount (float)] [-d]\n",
               argv[0]);
        exit(0);
      case 's':sscanf(optarg, "%ld", &blocksize);
        break;
      case 'c':sscanf(optarg, "%d", &block_count);
        break;
      case 'n':sscanf(optarg, "%d", &file_count);
        break;
    }
  int file_size = blocksize * block_count;
  read_times.reserve(file_count);
  write_times.reserve(file_count);
  printf("Creating %d files of size %d B", file_count, file_size);
  printf(", total data %.2f MB\n", (double) (file_size * file_count) / (1024.0 * 1024.0));

  int flags = O_CREAT;
  flags |= O_RDWR | O_APPEND | S_IFREG;
  mode_t mode = geteuid();
  auto *data = (unsigned char *) malloc(file_size);
  char filename[255];
  for (int i = 0; i < MAXREAD && i < blocksize; ++i)
    data[i] = i % 256;
  printf("Writting\n");
  std::vector<int> vec;
  vec.reserve(file_count);
  for (int i = 0; i < file_count; ++i) {
    vec.push_back(i);
  }
  for (int &it : vec) {
    sprintf(filename, "rand%d", it);
    int f = fs_->Open(filename, flags, mode);
    starttime();
    fs_->Write(f, data, file_size);
//    printf("File %s ", filename);
    gettime_w();
    fs_->Close(f);
  }
  printf("Total taken time to write files: %ldms \n", total_duration);
  printf("Average file write time: %ldms\n", total_duration / file_count);
  auto minimum = *std::min_element(write_times.begin(), write_times.end());
  auto maximum = *std::max_element(write_times.begin(), write_times.end());
  printf("Mimimum file write time; %ld\n", minimum);
  printf("Maximum file write time: %ld\n", maximum);
  printf("Reading\n");
  total_duration = 0;
  for (int &it : vec) {
    sprintf(filename, "rand%d", it);
    int f = fs_->Open(filename, flags, mode);
    starttime();
    fs_->Read(f, data, file_size);
//    printf("File %s ", filename);
    gettime_r();
    fs_->Close(f);
  }
//  gettime_r("Average read time", (int) file_count);
  printf("Total taken time to read files: %ldms \n", total_duration);
  printf("Average files read time: %ldms\n", total_duration / file_count);
  minimum = *std::min_element(read_times.begin(), read_times.end());
  maximum = *std::max_element(read_times.begin(), read_times.end());
  printf("Mimimum file read time; %ld\n", minimum);
  printf("Maximum file read time: %ld\n", maximum);

  fs_->DestroyFS();
  fs_.reset();
  free(data);
  return 0;
}