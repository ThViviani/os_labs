cmake_minimum_required(VERSION 3.10)

project(TemperatureMonitor C)

set(CMAKE_C_STANDARD 99)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/lib
)

set(SOURCES
        main.c
        src/database.c
        src/handlers.c
        src/serialport.c
        lib/cJSON.c
        lib/sqlite3.c
)

add_executable(TemperatureMonitor ${SOURCES})

add_executable(device_emulator device.c)

if(WIN32)
    target_link_libraries(TemperatureMonitor ws2_32)
endif ()
