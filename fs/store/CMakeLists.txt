cmake_minimum_required ( VERSION 3.12 FATAL_ERROR )
set ( CMAKE_CXX_STANDARD 17 )

set ( PROJECT_NAME "kvfs_store" )
project ( ${PROJECT_NAME} LANGUAGES CXX )

set ( STORE_SRCS
      ${PROJECT_SOURCE_DIR}/store.cpp )

source_group ( "Source Files" FILES ${STORE_SRCS} )

set ( STORE_HEADERS
      ${PROJECT_SOURCE_DIR}/store.hpp )

source_group ( " Header Files " FILES ${STORE_HEADERS} )

add_library (
        ${PROJECT_NAME} STATIC
        ${STORE_SRCS}
        ${STORE_HEADERS} )

target_link_libraries (
        ${PROJECT_NAME}
        PRIVATE
        kvfs_rocksdb
)
