/*
 * Copyright (c) 2019 Afshin Sabahi. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 *
 *      Author: Afshin Sabahi
 *      File:   path_res_test.cpp
 */

#include <filesystem>
#include <iostream>
#include <cstring>

bool starts_with(const std::string &s1, const std::string &s2) {
  return s2.size() <= s1.size() && s1.compare(0, s2.size(), s2) == 0;
}

int main() {

  std::filesystem::path file_path = "../../tmp/../../tmp/////db////..///.//file1.txt";

  std::cout << "Sample path: " + file_path.string() << std::endl;
  std::cout << "root path is: " << file_path.root_path() << std::endl;
  std::cout << "File name is: " << file_path.filename() << std::endl;

  if (file_path.is_relative()) {
    std::cout << "Attempting to resolve the relative path" << std::endl;
    std::filesystem::path pwd_ = "/";
    file_path = pwd_.append(file_path.string());

    std::cout << file_path.lexically_normal() << std::endl;
    file_path = file_path.lexically_normal();
  }

  std::filesystem::path resolved_path;
  for (const std::filesystem::path &e : file_path) {
    if (e == ".") {
      continue;
    }
    if (e == "..") {
      resolved_path = resolved_path.parent_path();
      continue;
    }
    resolved_path.append(e.string());
  }

  std::cout << std::filesystem::hash_value(resolved_path) << std::endl;
  std::cout << (resolved_path == file_path) << std::endl;
  return 0;
}