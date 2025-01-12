#!/bin/bash

# Остановить выполнение при ошибке
set -e

# Создание и переход в директорию сборки
mkdir -p build
cd build

# Конфигурация CMake
cmake ../

# Сборка проекта
cmake --build .

# Выполнение программы
echo "Запуск программы:"
./hello_world
