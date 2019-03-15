/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_io_test.cpp
 */

#include <kvfs/fs.h>
#include <kvfs/kvfs.h>
#include <random>

#undef KVFS_DEBUG

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

int main() {
  std::unique_ptr<FS> fs_ = std::make_unique<kvfs::KVFS>();
  int flags = O_CREAT;
  flags |= O_RDWR | O_APPEND;
  std::cout << "File opened with flags: " << flags << std::endl;
  mode_t mode = geteuid();
//  std::string file_name = "../..////./Hi.txt";
  std::string file_name = "Hi.txt";

  std::string data(100 * 1024 * 1024 , 'x'); // 25 bytes
  size_t data_size = 100 * 1024 * 1024;

  int fd_ = fs_->Open(file_name.c_str(), flags, mode);
//  std::cout << fd_ << std::endl;
//  std::cout << data_size << std::endl;
  /*random_bytes_engine rbe;
  std::vector<unsigned char> data(data_size);
  std::generate(begin(data), end(data), std::ref(rbe));*/

/*  ssize_t size = 0;
  for (int i =0; i < 100; ++i){
    size += fs_->Write(fd_, buffer_w, data_size);
    std::cout << "Wrote to kvfs: " << size << std::endl;
//    std::cout << data << std::endl;
  }

//  size = fs_->Write(fd_, buffer_w, data_size);
//  std::cout << "Wrote to kvfs: " << size << std::endl;

  void *buffer_r = std::malloc(data_size);
  auto status = fs_->PRead(fd_, buffer_r, data_size, 0);

  std::cout << "Read from kvfs: " << status << std::endl;
  std::cout << std::string(static_cast<char *>(buffer_r), data_size) << std::endl;*/

  //////////////////////////////////////////////

  int TOTAL_DATA = 100;

  const void *buffer_w = data.c_str();

  auto start = std::chrono::high_resolution_clock::now();

  ssize_t total_data_size = fs_->Write(fd_, buffer_w, data_size);

  /*for (int i = 0; i < TOTAL_DATA; i++) {

    ssize_t size = fs_->Write(fd_, buffer_w, data_size);

    total_data_size += size;

  }*/
  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "Wrote to kvfs: " << total_data_size << std::endl;
  total_data_size = 0;
  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms\n";

  /*for (int i = 0; i < TOTAL_DATA; ++i) {

    void *buffer_r = std::malloc(data_size);

    auto cur_read = fs_->PRead(fd_, buffer_r, data_size, total_data_size);
    free(buffer_r);

    total_data_size += cur_read;
  }*/
  void * buffer_r = std::malloc(data_size);
  std::cout << fs_->LSeek(fd_, 0, SEEK_SET) << std::endl;
  start = std::chrono::high_resolution_clock::now();
  total_data_size = fs_->Read(fd_, buffer_r ,data_size);
  finish = std::chrono::high_resolution_clock::now();
  std::cout << "Read from kvfs: " << total_data_size << std::endl;
  free(buffer_r);
  fs_->Close(fd_);

  std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "ms\n";

//  fs_->DestroyFS();
//  fs_->Remove(file_name.c_str());
  fs_->UnMount();
  fs_->Sync();
  fs_.reset();
  return 0;
}