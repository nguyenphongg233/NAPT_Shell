@echo off
REM TinyShell Test Scenario 2: Background Process Execution
REM This test runs processes in background and manages them

setlocal enabledelayedexpansion

echo.
echo ========== TEST SCENARIO 2: BACKGROUND PROCESSES ==========
echo.

echo Test 2.1: Starting test_process_b.exe in background
start /b test_process_b.exe > log_process_b.txt 2>&1

echo Process started. Checking process info...
echo.

echo Test 2.2: Running test_process_a.exe while background process runs
call test_process_a.exe

echo.
echo Test 2.3: Results from background process:
echo.
type log_process_b.txt

echo.
echo Cleaning up test files...
if exist log_process_b.txt del log_process_b.txt

echo.
echo ========== TEST SCENARIO 2 COMPLETED ==========
echo.
pause
