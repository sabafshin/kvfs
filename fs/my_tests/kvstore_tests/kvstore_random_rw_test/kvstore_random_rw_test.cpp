/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   kvstore_random_rw_test.cpp
 */
#include <kvfs_config.h>
#if KVFS_HAVE_LEVELDB
#include <kvfs_leveldb/kvfs_leveldb_store.h>
#endif
#if KVFS_HAVE_ROCKSDB
#include <kvfs_rocksdb/kvfs_rocksdb_store.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <chrono>
#include <fcntl.h>
#include <random>

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
void gettime_r(const char *desc, int repeat) {
  t_end = std::chrono::high_resolution_clock::now();
  long timedif = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
  read_times.push_back(timedif);
  total_duration += timedif;
}
void gettime_w(const char *desc, int repeat) {
  t_end = std::chrono::high_resolution_clock::now();
  long timedif = std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count();
  write_times.push_back(timedif);
  total_duration += timedif;
}

int main(int argc, char **argv) {
  // Setting some defaults
#if KVFS_HAVE_LEVELDB
  std::unique_ptr<kvfs::KVStore> kvstore_ = std::make_unique<kvfs::kvfsLevelDBStore>("/tmp/kvstore_test/");
#endif
#if KVFS_HAVE_ROCKSDB
  std::unique_ptr<kvfs::KVStore> kvstore_ = std::make_unique<kvfs::kvfsRocksDBStore>("/tmp/kvstore_test/");
#endif
  int64_t blocksize = 4096;
  int block_count = 10;
  int file_count = 10;
  int rvalue;

  while ((rvalue = getopt(argc, argv, "h--s:c:n:d")) != -1)
    switch (rvalue) {
      default:
        printf("Usage: %s [-s blocksize] [-c blockcount] [-n filecount]\n",
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

  char filename[256];
  printf("Writting\n");
  std::vector<int> vec;
  vec.reserve(file_count);
  for (int i = 0; i < file_count; ++i) {
    vec.push_back(i);
  }
  std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  const std::string value(file_size, 'x');
  for (auto it = vec.begin(); it != vec.end();) {
    sprintf(filename, "rand%d", *it);
    starttime();
    int f = kvstore_->Put(filename, value);
    gettime_w("write", file_count);
    vec.erase(it);
    std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  }

  printf("Total taken time to write files: %ldms \n", total_duration);
  printf("Average file write time: %ldms\n", total_duration / file_count);
  auto minimum = *std::min_element(write_times.begin(), write_times.end());
  auto maximum = *std::max_element(write_times.begin(), write_times.end());
  printf("Mimimum file write time; %ld\n", minimum);
  printf("Maximum file write time: %ld\n", maximum);
  printf("Reading\n");
  total_duration = 0;
  vec.reserve(file_count);
  for (int i = 0; i < file_count; ++i) {
    vec.push_back(i);
  }
  std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  for (auto it = vec.begin(); it != vec.end();) {
    sprintf(filename, "rand%d", *it);
    starttime();
    kvfs::KVStoreResult sr = kvstore_->Get(filename);
    gettime_r("read", file_count);
    vec.erase(it);
    std::shuffle(vec.begin(), vec.end(), std::mt19937(std::random_device()()));
  }
  printf("Total taken time to read files: %ldms \n", total_duration);
  printf("Average files read time: %ldms\n", total_duration / file_count);
  minimum = *std::min_element(read_times.begin(), read_times.end());
  maximum = *std::max_element(read_times.begin(), read_times.end());
  printf("Mimimum file read time; %ld\n", minimum);
  printf("Maximum file read time: %ld\n", maximum);

  printf("Removing\n");
  for (int i = 0; i < file_count; ++i) {
    sprintf(filename, "rand%d", i);
    kvstore_->Delete(filename);
  }
  kvstore_.reset();

  return 0;
}