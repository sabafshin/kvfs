## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_store")
project(${PROJECT_NAME} LANGUAGES CXX)

set(STORE_SRCS
    store.cpp
    store_entry.cpp
    store_result.cpp
    store_mutex.cpp store_cache.cpp store_cache.h)

source_group("Source Files" FILES ${STORE_SRCS})

set(STORE_HEADERS
    store.hpp
    store_entry.hpp
    store_result.hpp
    slice.hpp
    store_hash.h
    store_mutex.h store_cache.cpp store_cache.h)

source_group(" Header Files " FILES ${STORE_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${STORE_SRCS}
    ${STORE_HEADERS})

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_rocksdb
)
