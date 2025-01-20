#!/bin/bash

mkdir build
cd build

qmake ../

make -j$(nproc)