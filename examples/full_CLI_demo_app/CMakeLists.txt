cmake_minimum_required(VERSION 3.14)
project(full_CLI_demo_app CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/scibar)
endif()

add_executable(full_CLI_demo_app main.cpp)
target_link_libraries(full_CLI_demo_app PRIVATE scibar)

# Copy config.toml next to the executable so it's found by default.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.toml
    ${CMAKE_CURRENT_BINARY_DIR}/config.toml
    COPYONLY
)
