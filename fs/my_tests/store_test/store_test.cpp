/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_test.cpp
 */

#include <kvfs_rocksdb/rocksdb_store.h>
#include <kvfs_rocksdb/rocksdb_hash.h>
#include <inodes/inode_cache.h>
#include <inodes/directory_entry_cache.h>
#include <iostream>
#include <random>
#include <iomanip>

using namespace std;
using namespace kvfs;

/*void convert_to_hex_string(ostringstream &op,
                           const unsigned char* data, int size)
{
  // Format flags
  ostream::fmtflags old_flags = op.flags();

  // Fill characters
  char old_fill  = op.fill();
  op << hex << setfill('0');

  for (int i = 0; i < size; i++)
  {
    // Give space between two hex values
    if (i>0) {
      op << ' ';
    }

    // force output to use hex version of ascii code
    op << "0x" << setw(2) << static_cast<int>(data[i]);
  }

  op.flags(old_flags);
  op.fill(old_fill);
}

void convert_to_struct(istream& ip, unsigned char* data,
                       int size)
{
  // Get the line we want to process
  string line;
  getline(ip, line);

  istringstream ip_convert(line);
  ip_convert >> hex;

  // Read in unsigned ints, as wrote out hex version
  // of ascii code
  unsigned int u = 0;
  int i = 0;

  while ((ip_convert >> u) && (i < size)) {
    if((0x00 <= u) && (0xff >= u)) {
      data[i++] = static_cast<unsigned char>(u);
    }
  }
}*/

int main() {
  string name = "/tmp/db/";
  std::shared_ptr<Store> store_;

  store_ = std::make_shared<RocksDBStore>(name);

//  std::unique_ptr<inode_cache> i_cache = std::make_unique<inode_cache>(2, store_);
  auto *d_cache = new DentryCache(256);

  StoreEntryKey root{};
  auto seed = static_cast<uint32_t>(std::rand());
  root.inode_ = 0;
  root.hash_ = kvfs::XXH32(name.data(), static_cast<int>(name.size()), seed);

  std::cout << root.hash_ << std::endl;

  StoreEntryValue root_value;

  for (int i = 0; i < name.size(); ++i) {
    root_value.dirent_.d_name[i] = name[i];
  }

  root_value.fstat_.st_ino = 0;
  root_value.parent_hash_ = root.hash_;
  root_value.fstat_ = {
      1, 20, 3, 45, 0
  };

  std::cout << sizeof(kvfs_stat) << std::endl;
  std::cout << root_value.to_string().size() << std::endl;

  d_cache->insert(root, root_value);
  d_cache->insert(root, root_value);
  StoreEntryValue found{};
  auto foudn_bool = d_cache->find(root, found);
//
  std::cout << found.to_string() << std::endl;

//  ostringstream op;
//  convert_to_hex_string(op, reinterpret_cast<const unsigned char *>(&root_value), sizeof(StoreEntryValue));
//  string output = op.str();
//  cout << "After conversion from struct to hex string:\n"
//       << output << endl;
  auto root_slice = root.to_string();
  auto root_value_slice = root_value.to_string();
//  istringstream ip(root_value_slice.data());
//  StoreEntryValue back{};
//  convert_to_struct(ip, reinterpret_cast<unsigned char*>(&back), sizeof(StoreEntryValue));
  std::cout << root_value_slice.data() << std::endl;
  std::cout << root_value.to_string() << std::endl;
  bool status = store_->put(root.to_string(), root_value.to_string());

  if (status) {
    std::cout << "root insert success." << std::endl;

    bool haskey = store_->hasKey(root_slice);
    assert(haskey);
    StoreResult retrieve = store_->get(root_slice);
    retrieve.ensureValid();
    if (retrieve.isValid()) {
      std::cout << "root retrieve success." << std::endl;
//      delete name;
//      const auto *back = reinterpret_cast<const StoreEntryValue *>(retrieve.asString().data());
      string output = retrieve.asString();
      std::cout << output << std::endl;
//      istringstream ip(output);
      StoreEntryValue new_back{};
      new_back.parse(retrieve);
//      convert_to_struct(ip, reinterpret_cast<unsigned char*>(&new_back), sizeof(StoreEntryValue));
      std::cout << new_back.dirent_.d_name << std::endl;
      std::cout << new_back.fstat_.st_ino << std::endl;
      std::cout << new_back.parent_hash_ << std::endl;
      std::cout << new_back.fstat_.st_dev << std::endl;
      std::cout << new_back.fstat_.st_ino << std::endl;
//      std::cout << back.this_inode << std::endl;
    }
  }

  if (!status) {
    std::cout << "ERROR" << std::endl;
  }

//  d_cache.reset();


  store_->close();
//  std::cout << store_.use_count() << std::endl;
  delete d_cache;
//  i_cache.reset();
  store_.reset();
  return 0;
}