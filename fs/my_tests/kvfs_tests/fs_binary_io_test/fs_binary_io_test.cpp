/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   fs_binary_io_test.cpp
 */

#include <kvfs/fs.h>
#include <kvfs/kvfs.h>
#include <random>

int main() {

  std::unique_ptr<FS> kvfs_handler = std::make_unique<kvfs::KVFS>();

  char new_file_name[100] = "f1.bin";
  int new_file_flags = O_CREAT | O_RDWR | O_APPEND | S_IFREG;
  mode_t new_file_mode = geteuid();
//  size_t new_file_size = 100 * 1024 * 1024;
  size_t new_file_size = 4096;
  std::cout << "Generate sample random data" << std::endl;
  std::random_device engine;

  unsigned char random_char = engine();

  auto *write_buffer = (unsigned char *) calloc(new_file_size, sizeof(unsigned char));

  for (int i = 0; i < new_file_size; ++i) {
    *(write_buffer + i) = random_char;
//    printf("%d, ", *(write_buffer));
  }
//  printf("\n");
  std::cout << "kvfs performance test::" << std::endl;

  int fd_ = kvfs_handler->Open(new_file_name, new_file_flags, new_file_mode);
  auto start = std::chrono::high_resolution_clock::now();

  ssize_t total_data_written_size = kvfs_handler->Write(fd_, write_buffer, new_file_size);

  auto finish = std::chrono::high_resolution_clock::now();
  std::cout << "    Create '" << new_file_name << "', size = " << total_data_written_size;
  std::cout << ", in " << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count()
            << " microsec\n";

  kvfs_handler->LSeek(fd_, 0, SEEK_SET);
  unsigned char *read_buffer = (unsigned char *) calloc(new_file_size, sizeof(unsigned char));
  start = std::chrono::high_resolution_clock::now();
  int total_data_read_size = kvfs_handler->Read(fd_, read_buffer, new_file_size);

  finish = std::chrono::high_resolution_clock::now();
  std::cout << "    Read   '" << new_file_name << "', size = " << total_data_read_size;
  std::cout << ", in " << std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count()
            << " microsec\n";

//  for (int i = 0; i < new_file_size; ++i) printf("%d, ", *(read_buffer + i));
//  printf("\n");

  free(write_buffer);
  free(read_buffer);
  kvfs_handler->Close(fd_);
  kvfs_handler->Sync();
  kvfs_handler->Remove(new_file_name);

  auto dirstream = kvfs_handler->OpenDir("/");
  kvfs_dirent *dirent1 = kvfs_handler->ReadDir(dirstream);

  while (dirent1) {
    std::cout << dirent1->d_name << std::endl;
    dirent1 = kvfs_handler->ReadDir(dirstream);
  }

  kvfs_handler->CloseDir(dirstream);
  kvfs_handler->UnMount();
  kvfs_handler->DestroyFS();
  kvfs_handler.reset();
  return 0;

}