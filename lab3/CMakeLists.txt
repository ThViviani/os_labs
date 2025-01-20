cmake_minimum_required(VERSION 3.10)

project(SharedMemoryProject VERSION 1.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}include)

set(SOURCES
        main.c
        src/shared_memory.c
        src/utils.c
)

add_executable(SharedMemoryProject ${SOURCES})

target_include_directories(SharedMemoryProject PRIVATE include)