cmake_minimum_required (VERSION 3.10)
project (extalloc CXX)

option (COVERAGE_ENABLED "Code-coverage is enabled" No)

function (configure_target name)

    set_target_properties (${name} PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS OFF
    )
    
    if (MSVC)
        target_compile_options (${name} PRIVATE /W4)

        if (COVERAGE_ENABLED)
            message (WARNING "COVERAGE_ENABLED is not yet implemented for MSVC")
        endif (COVERAGE_ENABLED)

    elseif (CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options (${name} PRIVATE -Wall -pedantic)

        if (COVERAGE_ENABLED)
            target_compile_options (${name} PRIVATE -fprofile-arcs -ftest-coverage)
            target_link_libraries (${name} PUBLIC --coverage)
        endif (COVERAGE_ENABLED)

    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options (${name} PRIVATE
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
        )

        if (COVERAGE_ENABLED)
            target_compile_options (${name} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
            target_compile_options (${name} PRIVATE -fno-inline-functions)
            target_link_libraries (${name} PUBLIC -fprofile-instr-generate -fcoverage-mapping)
        endif (COVERAGE_ENABLED)

    else ()
        message (STATUS "Unknown compiler")
    endif ()

endfunction (configure_target)


############
# extalloc #
############

add_library (extalloc STATIC allocator.cpp allocator.hpp optional.hpp)
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

add_executable (unit-tests
    unit-tests.cpp
    test_optional.cpp
)
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


###############
# mmap_stress #
###############

add_executable (mmap_stress mmap_stress.cpp)
configure_target (mmap_stress)
target_link_libraries (mmap_stress PRIVATE extalloc)
