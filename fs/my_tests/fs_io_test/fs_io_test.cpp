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

using random_bytes_engine = std::independent_bits_engine<
    std::default_random_engine, CHAR_BIT, unsigned char>;

int main() {
  std::unique_ptr<FS> fs_ = std::make_unique<kvfs::KVFS>();
  int flags = O_CREAT;
  flags |= O_WRONLY;
  mode_t mode = geteuid();
//  std::string file_name = "../..////./Hi.txt";
  std::string file_name = "Hi.txt";

  int fd_ = fs_->Open(file_name.c_str(), flags, mode);
  std::cout << fd_ << std::endl;

  size_t data_size = 20000;
  std::cout << data_size << std::endl;

  random_bytes_engine rbe;
  std::vector<unsigned char> data(data_size);
  std::generate(begin(data), end(data), std::ref(rbe));

  const void *buffer_w = &data;

  ssize_t size = fs_->Write(fd_, buffer_w, data_size);
  std::cout << "Wrote to kvfs: " << size << std::endl;
//  size = fs_->Write(fd_, buffer_w, data_size);
//  std::cout << "Wrote to kvfs: " <<  size << std::endl;

  void *buffer_r = std::malloc(data_size);
  auto status = fs_->Read(fd_, buffer_r, data_size);

  std::cout << "Read from kvfs: " << status << std::endl;

  fs_->Close(fd_);

  free(buffer_r);
  fs_->TuneFS();
  fs_->DestroyFS();
  fs_.reset();
  return 0;
}