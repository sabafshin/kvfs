## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvstore_random_rw_test")
project(${PROJECT_NAME} LANGUAGES CXX)

set(TEST_SRCS
    kvstore_random_rw_test.cpp)
source_group("Source Files" FILES ${TEST_SRCS})

add_executable(
    ${PROJECT_NAME}
    ../../../../config/kvfs_config.h
    ../../../kvfs/fs_error.h
    ../../../kvfs/fs_error.cpp
    ${TEST_SRCS}
)

target_link_libraries(
    ${PROJECT_NAME}
    kvfs_leveldb
    kvfs_store
)