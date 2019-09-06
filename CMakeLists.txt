cmake_minimum_required(VERSION 3.14)

# Be verbose
set(CMAKE_VERBOSE_MAKEFILE ON)

# Prefer Clang over GCC
set(CMAKE_C_COMPILER_NAMES clang gcc icc cc)
set(CMAKE_CXX_COMPILER_NAMES clang++ g++ icpc c++ cxx)

project(ugg)

set(CMAKE_CXX_STANDARD 17)

# Search for the project modules in /build-aux/
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build-aux/")

# Helpers
include(AppendCompilerFlag)
include(GetCompilerArgs)
include(Trim)

# Default build type
# Based on https://blog.kitware.com/cmake-and-the-default-build-type/
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to `DEBUG' as none was specified.")
    set(CMAKE_BUILD_TYPE "DEBUG" CACHE STRING "Choose the type of build." FORCE)
    set(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

# Emit full debug info in clang
get_c_compiler_flags("" c_flags)
get_cxx_compiler_flags("" cxx_flags)
if("${c_flags}" MATCHES "( |^)-g( |$)")
    append_compiler_flag("-fno-omit-frame-pointer" "C")
    append_compiler_flag("-fno-limit-debug-info" "C")
endif()
if("${cxx_flags}" MATCHES "( |^)-g( |$)")
    append_compiler_flag("-fno-omit-frame-pointer" "CXX")
    append_compiler_flag("-fno-limit-debug-info" "CXX")
endif()

# Create the list of objects
add_subdirectory(lib/common)

add_executable(ugg ugg.cpp lib/cxxopts.hpp lib/cuckohash.hpp)
target_link_libraries(ugg PRIVATE libcommon)

get_c_compiler_flags(ugg c_flags)
get_cxx_compiler_flags(ugg cxx_flags)
message("Compiler C..........: ${CMAKE_C_COMPILER} ${c_flags}")
message("Compiler C++........: ${CMAKE_CXX_COMPILER} ${cxx_flags}")

