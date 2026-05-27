@echo off
REM TinyShell Test Scenarios - Main Menu
REM This script provides a menu to run various test scenarios

:menu
cls
echo.
echo ========== TinyShell Test Scenarios Menu ==========
echo.
echo Available Test Scenarios:
echo.
echo 1. Foreground Processes (scenario1_foreground.bat)
echo    - Tests running processes in foreground mode
echo    - Verifies shell waits for process completion
echo.
echo 2. Background Processes (scenario2_background.bat)
echo    - Tests running processes in background mode
echo    - Verifies shell continues while process runs
echo.
echo 3. Multiple Processes (scenario3_multiple.bat)
echo    - Tests running multiple processes simultaneously
echo    - Verifies system performance under load
echo.
echo 4. Exit
echo.
set /p choice="Select a test scenario (1-4): "

if "%choice%"=="1" (
    cls
    call scenario1_foreground.bat
    goto menu
) else if "%choice%"=="2" (
    cls
    call scenario2_background.bat
    goto menu
) else if "%choice%"=="3" (
    cls
    call scenario3_multiple.bat
    goto menu
) else if "%choice%"=="4" (
    exit /b 0
) else (
    echo Invalid choice. Please try again.
    timeout /t 2 > nul
    goto menu
)
