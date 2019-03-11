## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME "kvfs")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMake/")

include(CompilerSettingsUnix)
include(kvfsConfigChecks)

set(PACKAGE_VERSION "0.1")

configure_file(
    ${PROJECT_SOURCE_DIR}/CMake/kvfs-config.h.in
    config/kvfs_config.h
)

include_directories(${PROJECT_SOURCE_DIR}/config)
include_directories(${PROJECT_SOURCE_DIR}/fs)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/fs)

set(
    KVFS_SRCS
    fs/kvfs/kvfs.cpp
    fs/kvfs/fs_error.cpp)

source_group("Source Files" FILES ${KVFS_SRCS})

set(
    KVFS_HEADERS
    include/kvfs/kvfs.h
    include/kvfs/fs.h
    fs/kvfs/fs_error.h
    fs/kvfs/super.h
    fs/kvfs/kvfs_dirent.h)

source_group("Header Files" FILES ${KVFS_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${KVFS_SRCS}
    ${KVFS_HEADERS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_utils
    kvfs_rocksdb
    kvfs_store
    kvfs_inodes
    stdc++fs
)