@echo off
REM TinyShell Build Script for Windows with MinGW
REM This script rebuilds the TinyShell project
REM Run this from the TinyShell root directory

setlocal enabledelayedexpansion

echo Building TinyShell...
echo.

REM Store current directory
for /f "delims=" %%A in ('cd') do set "CURRENT_DIR=%%A"

REM Check if we're in the TinyShell directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found!
    echo Please run this script from the TinyShell root directory.
    echo.
    pause
    exit /b 1
)

REM Remove old build directory if it exists
if exist "build" (
    echo Removing old build directory...
    rmdir /s /q build
)

REM Create fresh build directory
echo Creating build directory...
mkdir build
cd build

REM Run CMake configuration
echo Configuring with CMake...
cmake -G "MinGW Makefiles" ..

if %ERRORLEVEL% neq 0 (
    echo.
    echo CMake configuration failed!
    echo.
    cd ..
    pause
    exit /b 1
)

REM Build the project
echo Building project...
mingw32-make

if %ERRORLEVEL% equ 0 (
    echo.
    echo ========================================
    echo Build completed successfully!
    echo Executable: %CURRENT_DIR%\build\TinyShell.exe
    echo ========================================
    echo.
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
    echo.
    cd ..
    pause
    exit /b 1
)

cd ..
pause
