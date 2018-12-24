set ( CMAKE_CXX_FLAGS_COMMON "-g -Wall -Wextra" )
set ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_COMMON}" )
set ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_COMMON} -O3" )


function ( apply_eden_compile_options_to_target THETARGET )
    target_compile_options ( ${THETARGET}
                             PUBLIC
                             -D_FILE_OFFSET_BITS=64
                             )
endfunction ()