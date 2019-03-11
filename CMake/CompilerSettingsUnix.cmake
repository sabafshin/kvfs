set ( CMAKE_CXX_FLAGS_COMMON "-g -Wall -Wextra" )
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_COMMON}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_COMMON} -Ofast")

add_compile_options("-std=c++17")
add_compile_options("-lstdc++fs")
add_link_options("-lstdc++fs")

function(apply_compile_options_to_target THETARGET)
    target_compile_options ( ${THETARGET}
                             PUBLIC
        -D_FILE_OFFSET_BITS=64)
endfunction ()