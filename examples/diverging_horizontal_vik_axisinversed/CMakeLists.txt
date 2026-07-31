cmake_minimum_required(VERSION 3.14)
project(diverging_horizontal_vik_axisinversed CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(diverging_horizontal_vik_axisinversed main.cpp)
target_link_libraries(diverging_horizontal_vik_axisinversed PRIVATE scibar)
