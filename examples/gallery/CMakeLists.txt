cmake_minimum_required(VERSION 3.14)
project(gallery CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(gallery main.cpp)
target_link_libraries(gallery PRIVATE scibar)
