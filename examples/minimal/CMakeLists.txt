cmake_minimum_required(VERSION 3.14)
project(scibar_minimal CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(scibar_minimal main.cpp)
target_link_libraries(scibar_minimal PRIVATE scibar)
