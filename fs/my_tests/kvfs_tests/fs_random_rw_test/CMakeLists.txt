## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "fs_random_rw_test")
project(${PROJECT_NAME} LANGUAGES CXX)

set(TEST_SRCS
    fs_random_rw_test.cpp)
source_group("Source Files" FILES ${TEST_SRCS})

add_executable(
    ${PROJECT_NAME}
    ${TEST_SRCS}
)

target_link_libraries(
    ${PROJECT_NAME}
    kvfs
)