## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "store_test")
project(${PROJECT_NAME} LANGUAGES CXX)

set(TEST_SRCS
    store_test.cpp)
source_group("Source Files" FILES ${TEST_SRCS})

#[[
set( TEST_HEADERS
     )
source_group ( " Header Files " FILES ${TEST_HEADERS} )
]]

add_executable(
    ${PROJECT_NAME}
    #        ${TEST_HEADERS}
    ${TEST_SRCS}
)

target_link_libraries(
    ${PROJECT_NAME}
    kvfs_store
    kvfs_utils
    kvfs_rocksdb
    kvfs_inodes
)