#include "../include/builtin.h"
#include "../include/process_mgr.h"
#include "../include/executor.h"
#include "../include/completion.h"
#include <iostream>
#include <windows.h>

bool HandleBuiltin(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    // 1. Exit/quit command
    if (args[0] == "exit" || args[0] == "quit") {
        ExitProcess(0);
    } 
    
    // 2. Help command
    else if (args[0] == "help") {
        std::cout << "\n";
        std::cout << "==============================================================================\n";
        std::cout << "=                    TinyShell - Command Reference                           =\n";
        std::cout << "==============================================================================\n\n";
        
        std::cout << "SHELL EXECUTION MODES:\n";
        std::cout << "  <command>         : Foreground mode (shell waits for process to finish)\n";
        std::cout << "  <command> &       : Background mode (process runs in new window, shell continues)\n\n";
        
        std::cout << "DIRECTORY & FILE OPERATIONS:\n";
        std::cout << "  cd <path>         : Change directory (supports '.', '..', Tab for suggestions)\n";
        std::cout << "  ls [path]         : List files (Unix-style)\n";
        std::cout << "  dir               : List files (Windows-style)\n\n";
        
        std::cout << "BACKGROUND PROCESS MANAGEMENT:\n";
        std::cout << "  list              : Show all background processes\n";
        std::cout << "  kill <PID>        : Terminate background process\n";
        std::cout << "  stop <PID>        : Suspend background process\n";
        std::cout << "  resume <PID>      : Resume suspended process\n\n";
        
        std::cout << "SYSTEM INFORMATION:\n";
        std::cout << "  date              : Display current date\n";
        std::cout << "  time              : Display current time\n";
        std::cout << "  path              : Show PATH environment variable\n";
        std::cout << "  addpath <path>    : Add directory to PATH\n\n";
        
        std::cout << "SHELL UTILITIES:\n";
        std::cout << "  help              : Display this help message\n";
        std::cout << "  cls               : Clear console screen\n";
        std::cout << "  exit / quit       : Exit the shell\n\n";
        
        std::cout << "FEATURES:\n";
        std::cout << "  Tab Completion    : Press TAB in 'cd' command for directory suggestions\n";
        std::cout << "  Signal Handling   : Press CTRL+C to terminate foreground process\n";
        std::cout << "  External Programs : Execute any program directly (notepad, calc, etc.)\n";
        std::cout << "  Batch Files       : Run .bat files directly\n\n";
        return true;
    }
    
    // 3. Clear screen command
    else if (args[0] == "cls") {
        system("cls"); 
        return true;
    }
    
    // 4. Change directory command
    else if (args[0] == "cd") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing path for cd command\n";
        } else {
            // SetCurrentDirectoryA automatically handles ".", ".." and absolute/relative paths
            if (!SetCurrentDirectoryA(args[1].c_str())) {
                std::cerr << "TinyShell: Error: Cannot change to this directory. Error code: " << GetLastError() << "\n";
            }
        }
        return true;
    }
    
    // 5. List directory command (Windows-style)
    else if (args[0] == "dir") {
        system("dir"); 
        return true;
    }
    
    // 5.5 List directory command (Unix-style)
    else if (args[0] == "ls") {
        if (args.size() < 2) {
            ListDirectory("");
        } else {
            ListDirectory(args[1]);
        }
        return true;
    }
    
    // 6. Date and time command
    else if (args[0] == "date" || args[0] == "time") {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (args[0] == "date") 
            std::cout << "Current date: " << st.wDay << "/" << st.wMonth << "/" << st.wYear << "\n";
        else 
            std::cout << "Current time: " << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "\n";
        return true;
    }
    
    // 7. Display PATH environment variable
    else if (args[0] == "path") {
        char buffer[32767];
        if (GetEnvironmentVariableA("PATH", buffer, 32767)) {
            std::cout << "PATH=" << buffer << "\n";
        } else {
            std::cerr << "TinyShell: Error: Cannot read PATH environment variable. Error code: " << GetLastError() << "\n";
        }
        return true;
    }
    
    // 8. Add directory to PATH
    else if (args[0] == "addpath") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing path argument for addpath command\n";
        } else {
            char buffer[32767];
            GetEnvironmentVariableA("PATH", buffer, 32767);
            std::string newPath = std::string(buffer) + ";" + args[1];
            if (SetEnvironmentVariableA("PATH", newPath.c_str())) {
                std::cout << "TinyShell: Successfully added path to PATH\n";
            } else {
                std::cerr << "TinyShell: Error: Failed to modify PATH. Error code: " << GetLastError() << "\n";
            }
        }
        return true;
    }
    
    // 9. List background processes
    else if (args[0] == "list") {
        ListProcesses();
        return true;
    }
    
    // 10. Kill process command
    else if (args[0] == "kill") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing PID for kill command\n";
        } else {
            KillProcess(std::stoul(args[1]));
        }
        return true;
    }
    
    // 11. Suspend process command
    else if (args[0] == "stop") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing PID for stop command\n";
        } else {
            StopProcess(std::stoul(args[1]));
        }
        return true;
    }
    
    // 12. Resume process command
    else if (args[0] == "resume") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing PID for resume command\n";
        } else {
            ResumeProcess(std::stoul(args[1]));
        }
        return true;
    }

    return false; // Return false if not a built-in command (pass to external process)
}