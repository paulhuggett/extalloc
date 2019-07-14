cmake_minimum_required (VERSION 3.10)
project (extalloc CXX)

function (configure_target name)

    set_target_properties (${name} PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
    )
    
    if (MSVC)
        target_compile_options (${name} PRIVATE /W4)
    elseif (CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options (${name} PRIVATE -Wall -pedantic)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options (${name} PRIVATE
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
        )
    else ()
        message (STATUS "Unknown compiler")
    endif ()

endfunction ()


############
# extalloc #
############

add_library (extalloc STATIC allocator.cpp allocator.hpp)
configure_target (extalloc)


###############
# google test #
###############

# Tell gtest to link against the "Multi-threaded Debug DLL runtime
# library" on Windows.
set (gtest_force_shared_crt ON CACHE BOOL "Always use msvcrt.dll")
add_subdirectory (googletest)


##############
# unit-tests #
##############

add_executable (unit-tests unit-tests.cpp)
configure_target (unit-tests)
target_link_libraries (unit-tests PRIVATE
    extalloc
    gtest_main
)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
   target_compile_options (unit-tests PRIVATE -Wno-global-constructors)
endif ()


##########
# stress #
##########

add_executable (stress main.cpp)
configure_target (stress)
target_link_libraries (stress PRIVATE extalloc)

set (out_xml "${CMAKE_BINARY_DIR}/unit-tests.xml")
add_custom_command (
    TARGET stress
    PRE_LINK
    COMMAND unit-tests "--gtest_output=xml:${out_xml}"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running unit-tests"
    DEPENDS unit-tests
    BYPRODUCTS "${out_xml}"
    VERBATIM
)
