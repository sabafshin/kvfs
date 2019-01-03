## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required ( VERSION 3.12 FATAL_ERROR )
set ( CMAKE_CXX_STANDARD 17 )

set ( PROJECT_NAME "kvfs" )
project ( ${PROJECT_NAME} LANGUAGES CXX )

set ( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/CMake/" )

set ( CMAKE_CXX_STANDARD 17 )
include ( CompilerSettingsUnix )
include ( kvfsConfigChecks )

set( PACKAGE_VERSION "0.1")

#[[include_directories(
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
)]]

#include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

configure_file(
        ${PROJECT_SOURCE_DIR}/CMake/kvfs-config.h.in
        config/kvfs_config.hpp
)



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

if ( WIN32 )
    set ( SYSTEM_LIBS ${SYSTEM_LIBS} Shlwapi.lib Rpcrt4.lib )
    set ( LIBS ${ROCKSDB_STATIC_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS} )
else ()
    set ( SYSTEM_LIBS ${CMAKE_THREAD_LIBS_INIT} )
    set ( LIBS ${ROCKSDB_SHARED_LIB} ${ROCKSDB_IMPORT_LIB} ${THIRDPARTY_LIBS} ${SYSTEM_LIBS} )
endif ()

set ( ROCKSDB_SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/rocksdb )

add_subdirectory ( ${ROCKSDB_SOURCE_DIR} )

include_directories ( ${ROCKSDB_SOURCE_DIR} )
include_directories ( ${ROCKSDB_SOURCE_DIR}/include )
########## RocksDB Config #########################################
#[[

########## Boost Config ###########################################
set ( Boost_USE_STATIC_LIBS OFF )
set ( Boost_USE_MULTITHREADED ON )
set ( Boost_USE_STATIC_RUNTIME OFF )

find_package ( Boost )
if ( Boost_FOUND )
    include_directories ( ${Boost_INCLUDE_DIRS} )
endif ()
########## Boost Config ###########################################

########## Folly Config ###########################################
find_package ( folly )
if ( folly_FOUND )
    include_directories ( ${folly_INCLUDE_DIRS} )
endif ()
########## Folly Config ###########################################

########### Fuse Config ############################################
#
#find_package ( FUSE REQUIRED )
#if ( FUSE_FOUND )
#    include_directories ( ${FUSE_INCLUDE_DIRS} )
#endif ()
########### Fuse Config ############################################
]]

#[[
########## kvfs Config ############################################

set ( HEADERS
      "${PROJECT_SOURCE_DIR}/include/kvfs/kvfs_wrong.h"
      include/kvfs/kv_store.hpp include/kvfs/kvfs_wd.hpp
      include/kvfs/kvfs_ad.h include/use_rocksdb/factory.hpp

      include/store/RocksDBHandles.hpp include/store/RocksDBException.hpp
      include/store/RocksDBStore.hpp

      include/store/StoreResult.hpp include/utils/properties.hpp include/kvfs/kvfs.hpp
      include/kvfs/kvfs_state.hpp include/kvfs/inode.hpp include/kvfs/inode_cache.hpp
      include/kvfs/dentry.hpp include/kvfs/kvfs_def.hpp config/kvfs_config.hpp )
source_group ( "Header Files" FILES ${HEADERS} )

set ( SOURCES
      ${PROJECT_SOURCE_DIR}/src/kvfs_wrong.cpp
      src/kvfs_ad.cpp src/db_functions.cpp
      src/store/RocksDBHandles.cpp src/store/RocksDBException.cpp
      src/store/RocksDBStore.cpp src/store/StoreResult.cpp src/utils/Properties.cpp
      include/kvfs/inode_mutex.hpp src/kvfs/kvfs.cpp src/kvfs/dentry.cpp
      src/kvfs/inode_cache.cpp src/kvfs/inode_mutex.cpp src/kvfs/kvfs_state.cpp )
source_group ( "Source Files" FILES ${SOURCES} )

include_directories ( ${PROJECT_SOURCE_DIR}/include )

add_executable ( ${PROJECT_NAME} ${ARTIFACT_SUFFIX}
                 ${HEADERS}
                 ${SOURCES}
                 )
########## kvfs Config ############################################

target_link_libraries ( ${PROJECT_NAME} ${LIBS} )
target_link_libraries ( ${PROJECT_NAME} ${SYSTEM_LIBS} )]]
add_subdirectory(${PROJECT_SOURCE_DIR}/fs)