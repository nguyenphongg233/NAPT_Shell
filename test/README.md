# TinyShell Test Suite Documentation

## Overview
This directory contains test programs and test scenarios for validating TinyShell functionality.

## Test Programs

### test_process_a.cpp / test_process_a.exe
- **Purpose**: Simple test process that displays process ID and performs sequential steps
- **Duration**: ~3 seconds
- **Output**: Shows 5 sequential steps with timing information
- **Use Case**: Testing foreground execution and basic process management

### test_process_b.cpp / test_process_b.exe
- **Purpose**: Long-running background test process
- **Duration**: ~15 seconds
- **Output**: Shows random number generation with timing
- **Use Case**: Testing background process execution and process management commands

### test_process_c.cpp / test_process_c.exe
- **Purpose**: CPU-intensive test process performing mathematical calculations
- **Duration**: ~3 seconds
- **Output**: Square root and power calculations with precision formatting
- **Use Case**: Testing process performance and resource management

### loop.exe
- **Purpose**: Simple infinite loop test (existing)
- **Use Case**: Testing CTRL+C signal handling and process termination

## Test Scenarios

### scenario1_foreground.bat
Tests foreground process execution:
- Runs test_process_a.exe and test_process_c.exe in sequence
- Verifies shell waits for each process to complete
- Total duration: ~6 seconds

### scenario2_background.bat
Tests background process execution:
- Starts test_process_b.exe in background
- Runs test_process_a.exe in foreground simultaneously
- Verifies shell continues while background process runs
- Demonstrates logging and process isolation

### scenario3_multiple.bat
Tests multiple simultaneous processes:
- Starts all three test processes in background
- Waits for completion and collects results
- Demonstrates system performance under concurrent load
- Tests resource management with multiple processes

### run_tests.bat
Interactive menu-driven test suite:
- Provides easy access to all test scenarios
- Allows selection and execution of individual tests
- Automatic cleanup of temporary files
- User-friendly navigation

## How to Compile Test Programs

From the test directory:
```bash
# Compile individual programs
g++ -o test_process_a.exe test_process_a.cpp
g++ -o test_process_b.exe test_process_b.cpp
g++ -o test_process_c.exe test_process_c.cpp
```

Or use the compile_tests.bat script (if available):
```bash
compile_tests.bat
```

## How to Run Tests

### Option 1: Using run_tests.bat (Recommended)
```bash
run_tests.bat
```
This provides an interactive menu to select and run test scenarios.

### Option 2: Run individual scenarios
```bash
scenario1_foreground.bat
scenario2_background.bat
scenario3_multiple.bat
```

### Option 3: Manual testing in TinyShell
```bash
# Foreground execution
run test_process_a.exe

# Background execution
run test_process_b.exe &

# Check background processes
list

# Kill a background process
kill <PID>
```

## Test Coverage

| Feature | Test | Scenario |
|---------|------|----------|
| Foreground execution | ✓ | 1, 2 |
| Background execution | ✓ | 2, 3 |
| Process information display | ✓ | 3 |
| Resource management | ✓ | 3 |
| Long-running processes | ✓ | 2, 3 |
| Sequential execution | ✓ | 1 |
| Concurrent execution | ✓ | 3 |
| CTRL+C handling | Manual | - |

## Expected Results

### Scenario 1: Should complete without errors
- Both processes should display their output sequentially
- Shell waits for each process to finish

### Scenario 2: Should complete without errors
- Background process (B) runs while foreground process (A) executes
- Log file from process B shows correct calculations
- All output should be visible in log files

### Scenario 3: Should complete without errors
- All three processes run concurrently
- Output from each process is captured correctly
- No resource conflicts or crashes

## Troubleshooting

### Processes fail to start
- Ensure all .exe files are compiled and in the test directory
- Check that paths are correct
- Verify compiler compatibility (MinGW)

### Test hangs
- Press CTRL+C to interrupt
- Check for infinite loops or blocking operations
- Review process output for errors

### Permission errors
- Ensure write permissions in test directory
- Check for antivirus interference
- Verify file access permissions
