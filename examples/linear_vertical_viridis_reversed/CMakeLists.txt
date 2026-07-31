cmake_minimum_required(VERSION 3.14)
project(linear_vertical_viridis_reversed CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(linear_vertical_viridis_reversed main.cpp)
target_link_libraries(linear_vertical_viridis_reversed PRIVATE scibar)
