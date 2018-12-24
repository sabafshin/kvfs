target_include_directories(
        edenfs
        PUBLIC
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
)

add_subdirectory(rocksdb)
add_subdirectory(store)
