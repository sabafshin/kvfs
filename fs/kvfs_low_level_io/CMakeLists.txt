## Copyright 2018 Afshin Sabahi. All rights reserved.
## Use of this source code is governed by a BSD-style
## license that can be found in the LICENSE file.

# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)

set(PROJECT_NAME "kvfs_low_level_io")
project(${PROJECT_NAME} LANGUAGES CXX)

set(LOW_LEVEL_SRCS
    )
source_group("Source Files" FILES ${LOW_LEVEL_SRCS})

set(LOW_LEVEL_HEADERS
    )
source_group(" Header Files " FILES ${LOW_LEVEL_HEADERS})

add_library(
    ${PROJECT_NAME} SHARED
    ${LOW_LEVEL_SRCS}
    ${LOW_LEVEL_HEADERS})

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    kvfs_store
)