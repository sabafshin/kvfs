## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_utils")
project(${PROJECT_NAME} LANGUAGES CXX)

set(UTILS_SRCS
    mutex_lock.cpp
    mutex.cpp)

source_group("Source Files" FILES ${UTILS_SRCS})

set(UTILS_HEADERS
    mutex_lock.h
    mutex.h)

source_group("Header Files" FILES ${UTILS_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${UTILS_SRCS}
    ${UTILS_HEADERS}
)

#[[
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_store
    kvfs_rocksdb
    kvfs_inodes
)]]
