/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_speed_test.cpp
 */

#include <store/store.h>
#include <kvfs_rocksdb/kvfs_rocksdb_store.h>
#include <iostream>

using namespace kvfs;

int main() {

  std::unique_ptr<Store> store_ = std::make_unique<RocksDBStore>("/tmp/db");

  system("dd if=/dev/urandom of=something.txt bs=3M count=1");
  auto fd_ = open("something.txt", O_RDONLY);
  auto data2 = malloc(3000000);
  read(fd_, data2, 3000000);
  close(fd_);
  char data[3000000]; // 25 bytes
//   size_t data_size = strlen(data);
  memcpy(data, data2, 3000000);
  auto start = std::chrono::high_resolution_clock::now();

  store_->put("key1", std::string(data, 3000000));
  StoreResult sr = store_->get("key1");
  if (sr.isValid()) {
    std::cout << sr.asString().size() << std::endl;
  }
  auto finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() << "ns\n";
  free(data2);
  store_->destroy();
  store_->close();
  store_.reset();
  std::filesystem::remove_all("/tmp/db");
//   system("rm -r /tmp/db");

  /*std::shared_ptr<RocksHandles> db_handle = std::make_shared<RocksHandles>("/tmp/db");

  start = std::chrono::high_resolution_clock::now();

  db_handle->db->Put(rocksdb::WriteOptions(), "key1", data);
  std::string ret;
//   db_handle->db->Get(ro, "key1", &ret);

  finish = std::chrono::high_resolution_clock::now();

  std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count() << "ns\n";

  db_handle.reset();

  std::filesystem::remove_all("/tmp/db");*/
  return 0;
}