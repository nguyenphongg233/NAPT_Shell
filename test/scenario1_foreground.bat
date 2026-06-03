@echo off
REM TinyShell Test Scenario 1: Foreground Process Execution
REM This test executes processes in foreground mode

echo.
echo ========== TEST SCENARIO 1: FOREGROUND PROCESSES ==========
echo Test 1.1: Running test_process_a.exe in foreground
echo.
call test_process_a.exe

echo.
echo Test 1.2: Running test_process_c.exe in foreground
echo.
call test_process_c.exe

echo.
echo ========== TEST SCENARIO 1 COMPLETED ==========
echo.
pause
