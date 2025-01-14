cmake_minimum_required(VERSION 3.10)

project(backgroundRunner)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(main main.c)

add_executable(test_program test_program.c)

add_subdirectory(lib)

target_link_libraries(main PRIVATE backgroundRunner)
