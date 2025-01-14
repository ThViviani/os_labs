cmake_minimum_required(VERSION 3.10)
project(backgroundRunner)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(backgroundRunner STATIC backgroundRunner.c)

target_include_directories(backgroundRunner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})