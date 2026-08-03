cmake_minimum_required(VERSION 3.14)
project(no_png CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(no_png main.cpp)
target_link_libraries(no_png PRIVATE scibar)
