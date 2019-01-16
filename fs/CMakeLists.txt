## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

cmake_minimum_required ( VERSION 3.12 FATAL_ERROR )
set ( CMAKE_CXX_STANDARD 17 )

add_subdirectory(kvfs_rocksdb)
add_subdirectory(store)
add_subdirectory(inodes)
add_subdirectory(store_test)
