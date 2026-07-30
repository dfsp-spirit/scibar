cmake_minimum_required(VERSION 3.10)
project(diverging_horizontal_example CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/../../src/core)
include_directories(SYSTEM ${CMAKE_SOURCE_DIR}/../../src/third_party)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(diverging_horizontal_example
    main.cpp
    ${CMAKE_SOURCE_DIR}/../../src/core/scibar_impl.cpp
)
target_compile_features(diverging_horizontal_example PRIVATE cxx_std_17)
