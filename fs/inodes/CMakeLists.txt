## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_inodes")
project(${PROJECT_NAME} LANGUAGES CXX)

set(INODES_SRCS
    inode_cache.cpp
    directory_entry_cache.cpp
    )

source_group("Source Files" FILES ${INODES_SRCS})

set(INODES_HEADERS
    inode_cache.h
    directory_entry_cache.h
    )

source_group("Header Files" FILES ${INODES_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${INODES_SRCS}
    ${INODES_HEADERS}
)
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_utils
)