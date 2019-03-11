## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(PROJECT_NAME "kvfs_leveldb")
project(${PROJECT_NAME} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

########## Leveldb configs ################################
option(LEVELDB_BUILD_TESTS "Build LevelDB's unit tests" OFF)
option(LEVELDB_BUILD_BENCHMARKS "Build LevelDB's benchmarks" OFF)
option(LEVELDB_INSTALL "Install LevelDB's header and library" OFF)

set(LEVELDB_LIB leveldb${ARTIFACT_SUFFIX})
set(LEVELDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../third_party/leveldb)
add_subdirectory(${LEVELDB_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../../third_party/leveldb)

include_directories(${LEVELDB_SOURCE_DIR}/include/)
#include_directories(${LEVELDB_SOURCE_DIR}/include/port)
###########################################################

set(KVFS_LEVELDB_STORE_SRCS
    kvfs_leveldb_handler.cpp
    kvfs_leveldb_exception.cpp
    kvfs_leveldb_store.cpp
    )

source_group("Source Files" FILES ${KVFS_LEVELDB_STORE_SRCS})

set(KVFS_LEVELDB_STORE_HEADERS
    kvfs_leveldb_handler.h
    kvfs_leveldb_exception.h
    kvfs_leveldb_store.h
    )

source_group(" Header Files " FILES ${KVFS_LEVELDB_STORE_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${KVFS_LEVELDB_STORE_SRCS}
    ${KVFS_LEVELDB_STORE_HEADERS}
)

target_link_libraries(
    ${PROJECT_NAME}
    LINK_PUBLIC
    ${LEVELDB_LIB}
    ${CMAKE_THREAD_LIBS_INIT}
)