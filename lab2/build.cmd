@echo off

REM Остановить выполнение при ошибке
setlocal enabledelayedexpansion
if errorlevel 1 exit /b %errorlevel%

REM Переход в директорию проекта
cd backgroundRunner || exit /b

REM Создание и переход в директорию сборки
if not exist build mkdir build
cd build

REM Конфигурация CMake
cmake -G "MinGW Makefiles" .. || exit /b

REM Сборка проекта
cmake --build . || exit /b

REM Запуск программы
echo Запуск программы:
backgroundRunner.exe