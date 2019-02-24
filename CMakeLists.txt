## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

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

########## RocksDB Config #########################################

#### Disable RocksDB test build ##########
option(WITH_GFLAGS "build with GFlags" OFF)
option(WITH_MD_LIBRARY "build with MD" OFF)
option(WITH_FALLOCATE "build with fallocate" OFF)
option(WITH_TESTS "build with tests" OFF)
option(WITH_TOOLS "build with tools" OFF)
##########################################

set(ROCKSDB_STATIC_LIB rocksdb${ARTIFACT_SUFFIX})
set(ROCKSDB_SHARED_LIB rocksdb-shared${ARTIFACT_SUFFIX})
set(ROCKSDB_IMPORT_LIB ${ROCKSDB_SHARED_LIB})

if (WIN32)
  set(SYSTEM_LIBS ${SYSTEM_LIBS} Shlwapi.lib Rpcrt4.lib)
  set(LIBS ${ROCKSDB_STATIC_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})
else ()
  set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT})
  set(LIBS ${ROCKSDB_SHARED_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})
endif ()

set(ROCKSDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/rocksdb)

add_subdirectory(${ROCKSDB_SOURCE_DIR})

include_directories(${ROCKSDB_SOURCE_DIR}/include)
include_directories(${ROCKSDB_SOURCE_DIR}/util)

########## RocksDB Config #########################################

include_directories(${PROJECT_SOURCE_DIR}/config)
include_directories(${PROJECT_SOURCE_DIR}/fs)
add_subdirectory(${PROJECT_SOURCE_DIR}/fs)

include_directories(${PROJECT_SOURCE_DIR}/include)
set(
    KVFS_SRCS
    fs/kvfs/kvfs.cpp
    fs/kvfs/fs_error.cpp fs/kvfs/fs_error.h)
source_group("Source Files" FILES ${KVFS_SRCS})

set(
    KVFS_HEADERS
    include/kvfs/kvfs.h
    include/kvfs/fs.h
    fs/kvfs/fs_error.cpp fs/kvfs/fs_error.h)
source_group("Header Files" FILES ${KVFS_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${KVFS_SRCS}
    ${KVFS_HEADERS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_store
    stdc++fs
)
