cmake_minimum_required(VERSION 3.14)
project(viridis_example CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(viridis_example main.cpp)
target_link_libraries(viridis_example PRIVATE scibar)
