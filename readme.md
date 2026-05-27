# TinyShell - A Simple Unix-like Shell

A lightweight shell implementation written in C++ that demonstrates core operating system concepts including process management, signal handling, and command parsing.

## Overview

TinyShell is an educational project for IT3070 (Operating Systems) that implements a basic UNIX-like shell with support for:
- Built-in commands
- External command execution
- Process management
- Signal handling
- Command parsing and execution

## Project Structure

```
.
├── include/              # Header files
│   ├── builtin.h        # Built-in commands
│   ├── completion.h     # Tab completion
│   ├── executor.h       # External command execution
│   ├── parser.h         # Command parsing
│   ├── process_mgr.h    # Process management
│   └── signal_hnd.h     # Signal handling
├── src/                  # Source files
│   ├── main.cpp         # Main shell loop
│   ├── builtin.cpp      # Built-in command implementations
│   ├── completion.cpp   # Tab completion implementation
│   ├── executor.cpp     # External command executor
│   ├── parser.cpp       # Command parser
│   ├── process_mgr.cpp  # Process manager
│   └── signal_hnd.cpp   # Signal handlers
├── test/                 # Test files
│   └── loop.cpp         # Loop test
├── build.bat            # Build script for Windows
├── CMakeLists.txt       # Build configuration
└── readme.md            # This file
```

## Requirements

- C++14 compiler (GCC, Clang)
- CMake 3.10+
- MinGW Make (on Windows)
- POSIX-compatible OS (Linux, macOS, WSL)

## Building

### Windows (with MinGW)

**Quick Build (Using Script):**
```bash
build.bat
```

**Manual Build:**
```bash
cd ..
rmdir /s /q build
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
mingw32-make
```

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
```

## Running

After building, run the shell:

```bash
./TinyShell  # Linux/macOS
TinyShell.exe  # Windows
```

## Features

### Built-in Commands
- **Shell Modes**: Foreground (waits for completion) and background (& suffix) - background processes open in new terminal window
- **Process Management**: `list`, `kill`, `stop`, `resume` for managing background processes
- **File Operations**: `ls` (Unix-style), `dir` (Windows-style), `cd`, `cd ..` for directory navigation
- **Directory Listing**: 
  - `ls` - List files in Unix style format
  - `ls <path>` - List files in specific directory
  - `dir` - List files in Windows style format
- **Tab Completion**: Press TAB while typing `cd <path>` to get directory suggestions
- **System Info**: `date`, `time`, `path`, `addpath` for system information
- **Utility Commands**: `help`, `cls`, `exit/quit`
- **External Execution**: Any command not listed above is executed directly as an external process

### Process Management
- Child process creation and monitoring with real-time status tracking
- Background process execution in new console windows with automatic cleanup
- Process suspension/resumption capabilities
- Proper handle and resource cleanup

### Signal Handling
- CTRL+C (SIGINT equivalent) - Terminate foreground processes safely
- Process termination event handling
- Signal-safe operations on Windows platform

### Tab Completion
- Auto-complete directory paths when using `cd` command
- Press TAB to see matching directories
- Supports both relative and absolute paths

## Usage

The shell operates as an interactive command-line interface. Type `help` to see all available commands.

### Quick Examples

```bash
help                        # Show all available commands
ls                          # List files (Unix-style)
ls C:\Windows               # List files in specific directory
dir                         # List files (Windows-style)
cd path/to/folder           # Change directory
cd C:\<TAB>                 # Press Tab for directory suggestions
notepad &                   # Run notepad in new window (background)
calc                        # Run calculator (foreground)
list                        # List all background processes
kill 1234                   # Terminate process with PID 1234
stop 5678                   # Suspend process with PID 5678
resume 5678                 # Resume suspended process
exit                        # Close the shell
```

For more detailed information, run `help` inside the shell.

## Notes

- This is an educational implementation designed to illustrate OS concepts rather than provide full shell functionality
- Uses Windows API for process management (Windows-specific)
- All code uses C++14 standard for compatibility
- Supports batch file (.bat) execution directly
- Process handles are properly managed to avoid resource leaks
- Background processes open in separate console windows using CREATE_NEW_CONSOLE flag
- Tab completion for `cd` command uses Windows file system APIs for fast directory lookup
- New terminal windows for background processes inherit parent environment variables and working directory
- External commands are executed directly without requiring a `run` prefix

## Author

IT3070 Operating Systems Course Project
