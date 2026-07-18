cmake_minimum_required(VERSION 3.10)
project(scibar_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths: core renderer headers and vendored GLM
include_directories(
    ${CMAKE_SOURCE_DIR}/../src/core
    ${CMAKE_SOURCE_DIR}
)
include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/../src/third_party
)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Source files for the renderer core library
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/../src/core/image.cpp
)

# Catch2 test runner (amalgamated)
add_executable(scibar_tests
    catch_amalgamated.cpp
    main.cpp
    test_image.cpp
    ${CORE_SOURCES}
)

target_compile_features(scibar_tests PRIVATE cxx_std_17)

# Output test images to a known directory
set_property(TARGET scibar_tests PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Enable testing with CTest
enable_testing()
add_test(NAME scibar_tests COMMAND scibar_tests)
