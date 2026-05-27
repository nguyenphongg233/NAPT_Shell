@echo off
REM Compile TinyShell Test Programs
REM This script compiles all C++ test programs

setlocal enabledelayedexpansion

echo Compiling TinyShell Test Programs...
echo.

REM Check if g++ is available
g++ --version > nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: g++ compiler not found!
    echo Please ensure MinGW is installed and g++ is in your PATH.
    echo.
    pause
    exit /b 1
)

REM Compile test programs
echo Compiling test_process_a.cpp...
g++ -o test_process_a.exe test_process_a.cpp
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile test_process_a.cpp
    pause
    exit /b 1
)
echo ✓ test_process_a.exe created

echo.
echo Compiling test_process_b.cpp...
g++ -o test_process_b.exe test_process_b.cpp
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile test_process_b.cpp
    pause
    exit /b 1
)
echo ✓ test_process_b.exe created

echo.
echo Compiling test_process_c.cpp...
g++ -o test_process_c.exe test_process_c.cpp
if %ERRORLEVEL% neq 0 (
    echo Error: Failed to compile test_process_c.cpp
    pause
    exit /b 1
)
echo ✓ test_process_c.exe created

echo.
echo ========================================
echo All test programs compiled successfully!
echo ========================================
echo.
echo Next step: Run 'run_tests.bat' to test TinyShell
echo.
pause
