## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_store")
project(${PROJECT_NAME} LANGUAGES CXX)

set(STORE_SRCS
    kvfs_store.cpp
    kvfs_store_result.cpp
    kvfs_store_entry.cpp)

source_group("Source Files" FILES ${STORE_SRCS})

set(STORE_HEADERS
    kvfs_store.h
    kvfs_store_entry.h
    kvfs_store_result.h
    )

source_group(" Header Files " FILES ${STORE_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${STORE_SRCS}
    ${STORE_HEADERS})