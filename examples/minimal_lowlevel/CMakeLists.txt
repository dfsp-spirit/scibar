cmake_minimum_required(VERSION 3.14)
project(minimal_lowlevel CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(minimal_lowlevel main.cpp)
target_link_libraries(minimal_lowlevel PRIVATE scibar)
