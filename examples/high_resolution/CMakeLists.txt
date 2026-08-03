cmake_minimum_required(VERSION 3.14)
project(high_resolution CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(high_resolution main.cpp)
target_link_libraries(high_resolution PRIVATE scibar)
