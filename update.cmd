@echo off

REM Проверка обновлений
echo Checking for updates...
git fetch origin

REM Получение хешей локальной и удалённой веток
for /f "delims=" %%i in ('git rev-parse HEAD') do set LOCAL=%%i
for /f "delims=" %%i in ('git rev-parse origin/main') do set REMOTE=%%i

REM Проверка на синхронность
if "%LOCAL%"=="%REMOTE%" (
    echo No updates available. The local branch is up-to-date with the remote branch.
    exit /b 0
)

REM Обновление проекта
echo Updates are available. Pulling changes from the remote branch...
git pull origin main

REM Успешное обновление
echo The project has been successfully updated!
exit /b 0
