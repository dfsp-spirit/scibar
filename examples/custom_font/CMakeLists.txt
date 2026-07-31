cmake_minimum_required(VERSION 3.14)
project(custom_font CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/scibar)
endif()

add_executable(custom_font main.cpp)
target_link_libraries(custom_font PRIVATE scibar)

# Copy the external font next to the executable so loadFont() can find it.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/LibertinusSerif-Regular.ttf
    ${CMAKE_CURRENT_BINARY_DIR}/LibertinusSerif-Regular.ttf
    COPYONLY
)
