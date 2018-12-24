include(FindPkgConfig)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if ( IS_DIRECTORY ${PROJECT_SOURCE_DIR}/third_party/rocksdb )
    set(KVFS_HAVE_ROCKSDB ${RocksDB_FOUND})
endif()
