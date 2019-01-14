/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   rocksdb_test.cpp
 */

#include "rocksdb/db.h"
#include <iostream>

int main() {
  const char *name = "/tmp/db/";
  rocksdb::DB *db;

  rocksdb::Options options;
  // Optimize RocksDB. This is the easiest way to get RocksDB to perform well.
  options.IncreaseParallelism();

  // Create the DB if it's not already present.
  options.create_if_missing = true;

  auto status = rocksdb::DB::Open(options, name, &db);

  db->Put(rocksdb::WriteOptions(), "root", name);

  std::string value;
  db->Get(rocksdb::ReadOptions(), "root", &value);

  std::cout << value << std::endl;

  db->Close();

  delete db;

  return 0;
}