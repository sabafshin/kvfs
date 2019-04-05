## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_NAME "kvfs")
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMake/")

include(CompilerSettingsUnix)
include(kvfsConfigChecks)

########### KVFS Configs ############################################
set(KVFS_VERSION_C "1.0")
set(KVFS_MAX_OPEN_FILES_C "512")
set(KVFS_CACHE_SIZE_C "512")
set(KVFS_BLOCK_SIZE_C "4096")
set(KVFS_MAX_HARDLINK_COUNT_C "1000")

option(BuildWithTests "Build Kvfs tests" ON)
option(BuildWithRocksDB "BuildWithRocksDB" OFF)
option(BuildWithLevelDB "BuildWithLevelDB" ON)
option(BuildWithThreadSafety "BuildWithThreadSafety" OFF)
#####################################################################

if (BuildWithRocksDB)
  ########## RocksDB Config #########################################
  #### Disable RocksDB test build ##########
  option(WITH_GFLAGS "build with GFlags" OFF)
  option(WITH_MD_LIBRARY "build with MD" OFF)
  option(WITH_FALLOCATE "build with fallocate" ON)
  option(WITH_TESTS "build with tests" OFF)
  option(WITH_TOOLS "build with tools" OFF)
  ##########################################
  set(ROCKSDB_STATIC_LIB rocksdb${ARTIFACT_SUFFIX})
  set(ROCKSDB_SHARED_LIB rocksdb-shared${ARTIFACT_SUFFIX})
  set(ROCKSDB_IMPORT_LIB ${ROCKSDB_SHARED_LIB})
  set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT})
  set(LIBS ${ROCKSDB_SHARED_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})
  set(ROCKSDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/rocksdb)
  add_subdirectory(${ROCKSDB_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/third_party/rocksdb)
  include_directories(${ROCKSDB_SOURCE_DIR})
  include_directories(${ROCKSDB_SOURCE_DIR}/include)
  ########## RocksDB Config #########################################

  set(KVFS_HAVE_ROCKSDB "1")
  message("-- Building with RocksDB as backing key-value store")
endif ()

if (BuildWithLevelDB)
  add_compile_options("-fPIC")
  ########## Leveldb configs ################################
  option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
  option(LEVELDB_BUILD_BENCHMARKS "Build LevelDB's benchmarks" ON)
  option(LEVELDB_INSTALL "Install LevelDB's header and library" OFF)
  option(BUILD_SHARED_LIBS ON)
  set(LEVELDB_LIB leveldb)
  set(LEVELDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/leveldb)
  add_subdirectory(${LEVELDB_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/third_party/leveldb)
  include_directories(${LEVELDB_SOURCE_DIR}/include/)
  ###########################################################

  set(KVFS_HAVE_LEVELDB "1")
  message("-- Building with LevelDB as backing key-value store")
endif ()

if (BuildWithThreadSafety)
  set(KVFS_THREAD_SAFE "1")
endif ()

configure_file(
    ${PROJECT_SOURCE_DIR}/CMake/kvfs_config.h.in
    ${PROJECT_SOURCE_DIR}/config/kvfs_config.h
)

include_directories(${PROJECT_SOURCE_DIR}/config)
include_directories(${PROJECT_SOURCE_DIR}/fs)
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/fs)

set(
    KVFS_SRCS
    fs/kvfs/kvfs.cpp
    fs/kvfs/fs_error.cpp
    fs/kvfs/super.cpp)

source_group("Source Files" FILES ${KVFS_SRCS})

set(
    KVFS_HEADERS
    include/kvfs/kvfs.h
    include/kvfs/fs.h
    fs/kvfs/fs_error.h
    fs/kvfs/super.h
    fs/kvfs/kvfs_dirent.h
    config/kvfs_config.h)

source_group("Header Files" FILES ${KVFS_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${KVFS_SRCS}
    ${KVFS_HEADERS}
)

if (BuildWithRocksDB)
  target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      kvfs_rocksdb
      kvfs_store
      kvfs_inodes
      stdc++fs
  )
endif ()
if (BuildWithLevelDB)
  target_link_libraries(
      ${PROJECT_NAME}
      PRIVATE
      kvfs_leveldb
      kvfs_store
      kvfs_inodes
      stdc++fs
  )
endif ()