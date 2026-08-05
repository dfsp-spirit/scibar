cmake_minimum_required(VERSION 3.14)
project(zero_friction_high_res CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(zero_friction_high_res main.cpp)
target_link_libraries(zero_friction_high_res PRIVATE scibar)
