cmake_minimum_required (VERSION 3.10)
project (extalloc CXX)

function(configure_target name)

    set_target_properties (${name} PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
    )
    
    if (MSVC)
        target_compile_options (${name} PUBLIC /W4)
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options (${name} PUBLIC -Wall -pedantic)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options (${name} PUBLIC -Weverything -Wno-c++98-compat)
    else ()
        message (STATUS "Unknown compiler")
    endif ()

endfunction ()


add_library (extalloc STATIC allocator.cpp allocator.hpp)
configure_target (extalloc)

# Tell gtest to link against the "Multi-threaded Debug DLL runtime
# library" on Windows.
set (gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
add_subdirectory (googletest)
add_executable (unit-tests unit-tests.cpp)
target_link_libraries (unit-tests PRIVATE extalloc gtest gtest_main)
configure_target (unit-tests)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options (unit-tests PRIVATE -Wno-global-constructors)
endif ()

add_executable (stress main.cpp)
target_link_libraries (stress PUBLIC extalloc)
configure_target (stress)

set (OUT_XML "${CMAKE_BINARY_DIR}/unit-tests.xml")
add_custom_command (
    TARGET stress
    PRE_LINK
    COMMAND unit-tests "--gtest_output=xml:${OUT_XML}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running unit-tests"
    DEPENDS unit-tests
    BYPRODUCTS ${OUT_XML}
    VERBATIM
)
