cmake_minimum_required(VERSION 3.14)
project(scibar VERSION 0.1.0 LANGUAGES CXX)

# -----------------------------------------------------------------------
# Verify that the version in scibar.hpp matches the CMake project version.
# This prevents the two from drifting out of sync.
# -----------------------------------------------------------------------
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/scibar/scibar.hpp" _scibar_ver_line
     REGEX "^#define SCIBAR_VERSION[^_]")
string(REGEX REPLACE ".*\"([0-9]+\\.[0-9]+\\.[0-9]+)\".*" "\\1"
       _scibar_header_ver "${_scibar_ver_line}")
if(NOT _scibar_header_ver STREQUAL PROJECT_VERSION)
    message(FATAL_ERROR
        "scibar.hpp version (${_scibar_header_ver}) does not match "
        "CMake project version (${PROJECT_VERSION}). "
        "Please update both simultaneously.")
endif()

# =========================================================================
# scibar — Single-Header Scientific Colorbar Rendering for C++17
# =========================================================================
#
# This CMake file defines an INTERFACE library target so consumers can
# write:
#
#   add_subdirectory(path/to/scibar)
#   target_link_libraries(my_app PRIVATE scibar)
#
# Or via FetchContent:
#
#   FetchContent_Declare(scibar GIT_REPOSITORY https://github.com/dfsp-spirit/scibar ...)
#   FetchContent_MakeAvailable(scibar)
#   target_link_libraries(my_app PRIVATE scibar)
#
# The library is header-only.  Exactly one translation unit (usually
# the consumer's main.cpp or a dedicated scibar_impl.cpp) must define
# SCIBAR_IMPLEMENTATION before including scibar.hpp.
# =========================================================================

add_library(scibar INTERFACE)

target_include_directories(scibar INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/core>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(scibar SYSTEM INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/third_party>
    $<INSTALL_INTERFACE:include/scibar/third_party>
)

target_compile_features(scibar INTERFACE cxx_std_17)

# -----------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------

option(SCIBAR_BUILD_EXAMPLES "Build example programs" OFF)
option(SCIBAR_BUILD_TESTS    "Build test suite"        OFF)

if(SCIBAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(cpp_tests)
endif()

if(SCIBAR_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
