cmake_minimum_required(VERSION 3.14)
project(tick_diagnostic CXX)

if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(tick_diagnostic main.cpp)
target_link_libraries(tick_diagnostic PRIVATE scibar)
