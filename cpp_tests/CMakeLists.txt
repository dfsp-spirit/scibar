cmake_minimum_required(VERSION 3.10)
project(scibar_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
include_directories(
    ${CMAKE_SOURCE_DIR}/../src/core
)
include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/../src/third_party
)

# Optimization flags
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Catch2 test runner (amalgamated)
add_executable(scibar_tests
    catch_amalgamated.cpp
    main.cpp
    test_scibar.cpp
)

target_compile_features(scibar_tests PRIVATE cxx_std_17)

# Output to build directory
set_property(TARGET scibar_tests PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Enable testing with CTest
enable_testing()
add_test(NAME scibar_tests COMMAND scibar_tests)
