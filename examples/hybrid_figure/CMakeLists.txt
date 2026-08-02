cmake_minimum_required(VERSION 3.14)
project(hybrid_figure CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/scibar)
endif()

add_executable(hybrid_figure main.cpp)
target_link_libraries(hybrid_figure PRIVATE scibar)

# Copy the reference PNG next to the executable so the SVG can
# reference it with a relative path (mainImageHref = "whole_brain_sulc.png").
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/whole_brain_sulc.png
    ${CMAKE_CURRENT_BINARY_DIR}/whole_brain_sulc.png
    COPYONLY
)
