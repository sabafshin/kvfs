## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(PROJECT_NAME "kvfs_leveldb")
project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
option(LEVELDB_BUILD_BENCHMARKS "Build LevelDB's benchmarks" OFF)
option(LEVELDB_INSTALL "Install LevelDB's header and library" OFF)

set(LEVELDB_LIB leveldb${ARTIFACT_SUFFIX})
set(LEVELDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../third_party/leveldb)
add_subdirectory(${ROCKSDB_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/third_party/leveldb)
include_directories(${LEVELDB_SOURCE_DIR}/include/)
include_directories(${LEVELDB_SOURCE_DIR}/include/port)