## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(PROJECT_NAME "kvfs_rocksdb")
project(${PROJECT_NAME} LANGUAGES CXX)

set(KVFS_ROCKSDB_STORE_SRCS
    kvfs_rocksdb_handler.cpp
    kvfs_rocksdb_exception.cpp
    kvfs_rocksdb_store.cpp
    )

source_group("Source Files" FILES ${KVFS_ROCKSDB_STORE_SRCS})

set(KVFS_ROCKSDB_STORE_HEADERS
    kvfs_rocksdb_handler.h
    kvfs_rocksdb_exception.h
    kvfs_rocksdb_store.h
    )

source_group(" Header Files " FILES ${KVFS_ROCKSDB_STORE_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${KVFS_ROCKSDB_STORE_SRCS}
    ${KVFS_ROCKSDB_STORE_HEADERS}
)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${LIBS}
    ${SYSTEM_LIBS}
)