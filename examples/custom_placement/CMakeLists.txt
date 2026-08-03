cmake_minimum_required(VERSION 3.14)
project(custom_placement CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(custom_placement main.cpp)
target_link_libraries(custom_placement PRIVATE scibar)
