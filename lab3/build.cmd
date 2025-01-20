@echo off

cd cross_platform_logger

mkdir build
cd build

cmake -G "MinGW Makefiles" ..

cmake --build .

cross_platform_logger.exe