cmake_minimum_required(VERSION 3.10)

project(temp_logger)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(main temperature_logger.c)
add_executable(device_emulator device.c)