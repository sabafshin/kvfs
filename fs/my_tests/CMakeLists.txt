## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(path_resolution_test)
add_subdirectory(os_filesystem_test)
add_subdirectory(kvfs_tests/fs_binary_io_test)
add_subdirectory(kvfs_tests/fs_nested_directories_test)
add_subdirectory(kvfs_tests/fs_random_rw_test)
add_subdirectory(kvfs_tests/fs_seq_rw_test)
add_subdirectory(kvstore_tests)