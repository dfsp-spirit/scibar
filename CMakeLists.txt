cmake_minimum_required(VERSION 3.14)
project(scibar VERSION 0.2.1 LANGUAGES CXX)

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
# This CMake file defines an INTERFACE library target.  Consumers can
# use the library in three ways:
#
#  1. add_subdirectory:
#       add_subdirectory(path/to/scibar)
#       target_link_libraries(my_app PRIVATE scibar::scibar)
#
#  2. FetchContent:
#       include(FetchContent)
#       FetchContent_Declare(scibar GIT_REPOSITORY https://github.com/dfsp-spirit/scibar ...)
#       FetchContent_MakeAvailable(scibar)
#       target_link_libraries(my_app PRIVATE scibar::scibar)
#
#  3. find_package (after cmake --install):
#       find_package(scibar 0.1.0 REQUIRED)
#       target_link_libraries(my_app PRIVATE scibar::scibar)
#
# The library is header-only.  Exactly one translation unit (usually
# the consumer's main.cpp or a dedicated scibar_impl.cpp) must define
# SCIBAR_IMPLEMENTATION before including scibar.hpp.
# =========================================================================

add_library(scibar INTERFACE)
add_library(scibar::scibar ALIAS scibar)

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
# Install rules
# -----------------------------------------------------------------------

include(GNUInstallDirs)

install(TARGETS scibar EXPORT scibarTargets)

install(DIRECTORY src/core/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

install(DIRECTORY src/third_party/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/scibar/third_party
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# -----------------------------------------------------------------------
# CMake package config — enables find_package(scibar) after install
# -----------------------------------------------------------------------

include(CMakePackageConfigHelpers)

install(EXPORT scibarTargets
    FILE scibarTargets.cmake
    NAMESPACE scibar::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/scibar
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/scibarConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/scibarConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/scibar
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/scibarConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/scibarConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/scibarConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/scibar
)

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
