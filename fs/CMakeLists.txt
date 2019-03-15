## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

if (BuildWithRocksDB)
  add_subdirectory(kvfs_rocksdb)
endif ()

if (BuildWithLevelDB)
  add_subdirectory(kvfs_leveldb)
endif ()

add_subdirectory(store)
add_subdirectory(inodes)
add_subdirectory(kvfs_utils)

if (BuildWithTests)
  add_subdirectory(my_tests)
endif ()