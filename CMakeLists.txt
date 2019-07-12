cmake_minimum_required (VERSION 3.10)
project (extalloc CXX)
add_executable (extalloc
    allocator.cpp
    allocator.hpp
    main.cpp
)
set_target_properties (extalloc PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
)

if (MSVC)
    target_compile_options (extalloc PUBLIC /W4)
elseif (CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options (extalloc PUBLIC -Wall -pedantic)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options (extalloc PUBLIC -Weverything -Wno-c++98-compat)
else ()
     message (STATUS "Unknown compiler")
endif ()
