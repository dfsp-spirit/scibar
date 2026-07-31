cmake_minimum_required(VERSION 3.14)
project(scibar_tests CXX)

# Bring in scibar INTERFACE target if building standalone
if(NOT TARGET scibar)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. "${CMAKE_CURRENT_BINARY_DIR}/scibar")
endif()

add_executable(scibar_tests
    catch_amalgamated.cpp
    main.cpp
    test_scibar.cpp
)

target_link_libraries(scibar_tests PRIVATE scibar)

set_property(TARGET scibar_tests PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

enable_testing()
add_test(NAME scibar_tests COMMAND scibar_tests)
