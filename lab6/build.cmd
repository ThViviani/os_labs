@echo off

mkdir build
cd build

qmake lab6.pro
make

cd release
lab6.exe