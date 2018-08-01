# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("kvfs")

# Include sub-projects.
set(rocksdb_DIR ${PROJECT_SOURCE_DIR}/rocksdb)
set(kvfs_DIR ${PROJECT_SOURCE_DIR}/kvfs)

add_subdirectory (${rocksdb_DIR})
add_subdirectory (${kvfs_DIR})

#add_subdirectory("rocksdb")

#set(ROCKSDB_INCLUDE $${PROJECT_SOURCE_DIR}/rocksdb/include)

#include_directories(${ROCKSDB_INCLUDE})
#add_dependencies("kvfs" "rocksdb")

#option(WITH_MD_LIBRARY "build with MD" OFF)
#option(WITH_FALLOCATE "build with fallocate" OFF)
#option(WITH_TESTS "build with tests" OFF)
#option(WITH_TOOLS "build with tools" OFF)
#option(WITH_GFLAGS "build with GFlags" OFF)

# Rocksdb integration
#set (THIRDPARTY_LIBS "")         # Initialization, don't touch
#
# Defaults
#
#set(ROCKSDB_HOME $ENV{THIRDPARTY_HOME}/rocksdb/)
#set(ROCKSDB_INCLUDE ${ROCKSDB_HOME}/build/Linux-Debug/rocksdb/include)
#set(ROCKSDB_LIB_DEBUG ${ROCKSDB_HOME}/lib/native/debug/amd64/rocksdb.lib)
#set(ROCKSDB_LIB_RELEASE ${ROCKSDB_HOME}/lib/native/retail/amd64/rocksdb.lib)

#include_directories(${ROCKSDB_INCLUDE})
include_directories(${rocksdb_DIR}/include DESTINATION include)


