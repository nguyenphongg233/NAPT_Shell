@echo off
REM TinyShell Test Scenario 3: Multiple Process Execution
REM This test runs multiple processes to test system performance

setlocal enabledelayedexpansion

echo.
echo ========== TEST SCENARIO 3: MULTIPLE PROCESSES ==========
echo.

REM Start multiple background processes
echo Test 3.1: Starting multiple test processes in background
start /b test_process_a.exe > log_a.txt 2>&1
start /b test_process_b.exe > log_b.txt 2>&1
start /b test_process_c.exe > log_c.txt 2>&1

echo All processes started. Waiting for completion...
echo.

:wait_loop
tasklist | find "test_process_b.exe" > nul
if %ERRORLEVEL% equ 0 (
    echo Waiting for background processes...
    timeout /t 2 /nobreak > nul
    goto wait_loop
)

echo.
echo Test 3.2: Collecting results from all processes
echo.
echo ===== Results from Process A =====
type log_a.txt
echo.
echo ===== Results from Process B =====
type log_b.txt
echo.
echo ===== Results from Process C =====
type log_c.txt
echo.

REM Cleanup
echo Cleaning up test files...
if exist log_a.txt del log_a.txt
if exist log_b.txt del log_b.txt
if exist log_c.txt del log_c.txt

echo.
echo ========== TEST SCENARIO 3 COMPLETED ==========
echo.
pause
