/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   store_test.cpp
 */

#include "../kvfs_rocksdb/rocksdb_store.hpp"
#include "../kvfs_rocksdb/rocksdb_hash.hpp"
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
  const char *name = "/tmp/db/";
  std::unique_ptr<Store> store_;

  store_ = std::make_unique<RocksDBStore>(name);

  dir_key root{};
  auto seed = static_cast<uint32_t>(std::rand());
  root.inode = 0;
  root.hash = kvfs::XXH32(name, static_cast<int>(strlen(name)), seed);

  std::cout << root.hash << std::endl;

  dir_value root_value{"/tmp/db/"};

  root_value.this_inode = 0;
  root_value.parent_hash = root.hash;
  root_value.fstat = {
      1, 20, 3, 45, 0
  };

  std::cout << sizeof(kvfs_stat) << std::endl;
  std::cout << root_value.to_slice().size() << std::endl;

  /*ostringstream op;
  convert_to_hex_string(op, reinterpret_cast<const unsigned char *>(&root_value), sizeof(dir_value));
  string output = op.str();
  cout << "After conversion from struct to hex string:\n"
       << output << endl;
*/
  auto root_slice = root.to_slice();
  auto root_value_slice = root_value.to_slice();
//  istringstream ip(root_value_slice.data());
//  dir_value back{};
//  convert_to_struct(ip, reinterpret_cast<unsigned char*>(&back), sizeof(dir_value));
  std::cout << root_value_slice.data() << std::endl;
  bool status = store_->put(root.to_slice(), root_value.to_slice());

  if (status) {
    std::cout << "root insert success." << std::endl;

    bool haskey = store_->hasKey(root_slice);
    assert(haskey);
    StoreResult retrieve = store_->get(root_slice);
    retrieve.ensureValid();
    if (retrieve.isValid()) {
      std::cout << "root retrieve success." << std::endl;
//      delete name;
//      const auto *back = reinterpret_cast<const dir_value *>(retrieve.asString().data());
      string output = retrieve.asString();
      std::cout << output << std::endl;
//      istringstream ip(output);
      dir_value new_back{};
      new_back.parse(retrieve);
//      convert_to_struct(ip, reinterpret_cast<unsigned char*>(&new_back), sizeof(dir_value));
      std::cout << new_back.name << std::endl;
      std::cout << new_back.this_inode << std::endl;
      std::cout << new_back.parent_hash << std::endl;
      std::cout << new_back.fstat.st_dev << std::endl;
      std::cout << new_back.fstat.st_ino << std::endl;
//      std::cout << back.this_inode << std::endl;
    }
  }

  if (!status) {
    std::cout << "ERROR" << std::endl;
  }

  store_->close();

  store_.reset();
  return 0;
}