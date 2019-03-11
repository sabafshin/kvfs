## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_rocksdb")
project(${PROJECT_NAME} LANGUAGES CXX)

#### Disable RocksDB test build ##########
option ( WITH_GFLAGS "build with GFlags" OFF )
option ( WITH_MD_LIBRARY "build with MD" OFF )
option ( WITH_FALLOCATE "build with fallocate" OFF )
option ( WITH_TESTS "build with tests" OFF )
option ( WITH_TOOLS "build with tools" OFF )
##########################################

########## RocksDB Config #########################################
set ( ROCKSDB_STATIC_LIB rocksdb${ARTIFACT_SUFFIX} )
set ( ROCKSDB_SHARED_LIB rocksdb-shared${ARTIFACT_SUFFIX} )
set ( ROCKSDB_IMPORT_LIB ${ROCKSDB_SHARED_LIB} )

#[[if ( WIN32 )
    set ( SYSTEM_LIBS ${SYSTEM_LIBS} Shlwapi.lib Rpcrt4.lib )
    set ( LIBS ${ROCKSDB_STATIC_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS} )
else ()]]
    set ( SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT} )
    set ( LIBS ${ROCKSDB_SHARED_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS} )
#endif ()

set(ROCKSDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../third_party/rocksdb)

add_subdirectory(${ROCKSDB_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/third_party/rocksdb)

include_directories ( ${ROCKSDB_SOURCE_DIR} )
include_directories ( ${ROCKSDB_SOURCE_DIR}/include )
########## RocksDB Config #########################################

set(ROCKSDB_STORE_SRCS
    rocksdb_handler.cpp
    rocksdb_exception.cpp
    #    rocksdb_hash.cpp
    rocksdb_store.cpp
    )

source_group("Source Files" FILES ${ROCKSDB_STORE_SRCS})

set(ROCKSDB_STORE_HEADERS
    rocksdb_handler.h
    rocksdb_exception.h
    #    rocksdb_hash.h
    rocksdb_store.h
    )

source_group(" Header Files " FILES ${ROCKSDB_STORE_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${ROCKSDB_STORE_SRCS}
    ${ROCKSDB_STORE_HEADERS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    ${LIBS}
    ${SYSTEM_LIBS}
)