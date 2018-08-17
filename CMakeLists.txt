## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

SET(PROJECT_NAME "kvfs")
PROJECT(${PROJECT_NAME})

#### Disable RocksDB test build ##########
option(WITH_GFLAGS "build with GFlags" OFF)
option(WITH_MD_LIBRARY "build with MD" OFF)
option(WITH_FALLOCATE "build with fallocate" OFF)
option(WITH_TESTS "build with tests" OFF)
option(WITH_TOOLS "build with tools" OFF)
##########################################

########## RocksDB Config #########################################
set(ROCKSDB_STATIC_LIB rocksdb${ARTIFACT_SUFFIX})
set(ROCKSDB_SHARED_LIB rocksdb-shared${ARTIFACT_SUFFIX})
set(ROCKSDB_IMPORT_LIB ${ROCKSDB_SHARED_LIB})

if(WIN32)
  set(SYSTEM_LIBS ${SYSTEM_LIBS} Shlwapi.lib Rpcrt4.lib)
  set(LIBS ${ROCKSDB_STATIC_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})
else()
  set(SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT})
  set(LIBS ${ROCKSDB_SHARED_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS})
endif()

set (ROCKSDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/rocksdb)

add_subdirectory(${ROCKSDB_SOURCE_DIR})

include_directories(${ROCKSDB_SOURCE_DIR})
include_directories(${ROCKSDB_SOURCE_DIR}/include) 
########## RocksDB Config #########################################

########## KVFS Config ############################################

#SET(HEADERS
#  "${PROJECT_SOURCE_DIR}/include/kvfs/kvfs.h"
#)
#SOURCE_GROUP("Header Files" FILES ${HEADERS})

SET(SOURCES
  ${PROJECT_SOURCE_DIR}/src/kvfs.cpp
)
SOURCE_GROUP("Source Files" FILES ${SOURCES})

include_directories(${PROJECT_SOURCE_DIR}/include)

ADD_EXECUTABLE(${PROJECT_NAME} ${ARTIFACT_SUFFIX} 
  #${HEADERS}
  ${SOURCES}
)
########## KVFS Config ############################################

TARGET_LINK_LIBRARIES(${PROJECT_NAME}  ${LIBS})