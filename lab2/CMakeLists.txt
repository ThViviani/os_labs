cmake_minimum_required(VERSION 3.10)

project(backgroundRunner)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(backgroundRunner backgroundRunner.c)

add_executable(main main.c)

add_executable(test_program test_program.c)

target_link_libraries(main backgroundRunner)
