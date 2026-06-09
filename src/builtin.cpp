#include "../include/builtin.h"
#include "../include/process_mgr.h"
#include "../include/executor.h"
#include "../include/completion.h"
#include <iostream>
#include <windows.h>
#include <iomanip>
#include <stdexcept>

bool HandleBuiltin(const std::vector<std::string>& args) {
    if (args.empty()) return false;

    // 1. Exit/quit command
    if (args[0] == "exit" || args[0] == "quit") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for exit/quit command\n";
            return true;
        }
        ExitProcess(0);
    } 
    
    // 2. Help command
    else if (args[0] == "help") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for help command\n";
            return true;    
        }
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
        
        std::cout << "SYSTEM ENVIRONMENT:\n";
        std::cout << "  date              : Display current date\n";
        std::cout << "  time              : Display current time\n";
        std::cout << "  path              : Show PATH environment variable\n";
        std::cout << "  addpath <path>    : Add directory to PATH\n";
        std::cout << "  removepath <path> : Safely remove directory from PATH\n";
        std::cout << "  export VAR=VAL    : Set/Update environment variable (Alias: set)\n\n";
        
        std::cout << "SHELL UTILITIES:\n";
        std::cout << "  help              : Display this help message\n";
        std::cout << "  cls               : Clear console screen\n";
        std::cout << "  exit / quit       : Exit the shell\n\n";
        
        std::cout << "FEATURES:\n";
        std::cout << "  Tab Completion    : Press TAB in 'cd' command for directory suggestions\n";
        std::cout << "  Signal Handling   : Press CTRL+C to terminate foreground process\n";
        std::cout << "  External Programs : Execute any program directly (notepad, calc, etc.)\n";
        std::cout << "  Batch Files       : Run .bat files directly\n";
        std::cout << "  Pipes & Redir     : Supports |, >, < for process routing\n\n";
        return true;
    }
    
    // 3. Clear screen command
    else if (args[0] == "cls") {
        system("cls"); 
        return true;
    }
    
    // 4. Change directory command
    else if (args[0] == "cd") {
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for cd command\n";
            return true;    
        }
        if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing path for cd command\n";
        } 
        else {
            // SetCurrentDirectoryA automatically handles ".", ".." and absolute/relative paths
            if (!SetCurrentDirectoryA(args[1].c_str())) {
                std::cerr << "TinyShell: Error: Cannot change to this directory. Error code: " << GetLastError() << "\n";
            }
        }
        return true;
    }
    
    // 5. List directory command (Windows-style)
    else if (args[0] == "dir") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for dir command\n";
            return true;    
        }
        system("dir"); 
        return true;
    }
    
    // 5.5 List directory command (Unix-style)
    else if (args[0] == "ls") {
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for ls command\n";
            return true;    
        }
        if (args.size() < 2) {
            ListDirectory("");
        } 
        else {
            ListDirectory(args[1]);
        }
        return true;
    }
    
    // 6. Date and time command
    else if (args[0] == "date" || args[0] == "time") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for date/time command\n";
            return true;    
        }
        
        SYSTEMTIME st;
        GetLocalTime(&st);
        
        if (args[0] == "date") {
            std::cout << "Current date: " 
                      << std::setfill('0') << std::setw(2) << st.wDay << "/" 
                      << std::setfill('0') << std::setw(2) << st.wMonth << "/" 
                      << st.wYear << "\n";
        } else {
            std::cout << "Current time: " 
                      << std::setfill('0') << std::setw(2) << st.wHour << ":" 
                      << std::setfill('0') << std::setw(2) << st.wMinute << ":" 
                      << std::setfill('0') << std::setw(2) << st.wSecond << "\n";
        }
        return true;
    } 

    // 7. Display PATH environment variable
    else if (args[0] == "path") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for path command\n";
            return true;    
        }
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
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for addpath command\n";
            return true;    
        }
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

    // 8.1 Remove directory from PATH
    else if (args[0] == "removepath") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Usage: removepath <path_to_remove>\n";
            return true;
        }

        // Reconstruct target path (handle spaces in folder names)
        std::string targetPath = args[1];
        for (size_t i = 2; i < args.size(); i++) targetPath += " " + args[i];

        char currentPath[32767];
        if (GetEnvironmentVariableA("PATH", currentPath, sizeof(currentPath))) {
            std::string pathStr(currentPath);

            // The Trick: Wrap both PATH and Target in semicolons to ensure exact matches only
            std::string searchStr = ";" + pathStr + ";";
            std::string targetSearch = ";" + targetPath + ";";

            size_t pos = searchStr.find(targetSearch);
            if (pos != std::string::npos) {
                // Replace the found exact match with a single semicolon to stitch it back together
                searchStr.replace(pos, targetSearch.length(), ";");

                // Clean up the temporary wrapping semicolons at the start and end
                if (searchStr.length() > 0 && searchStr.front() == ';') searchStr.erase(0, 1);
                if (searchStr.length() > 0 && searchStr.back() == ';') searchStr.pop_back();

                // Apply updated PATH back to environment
                if (SetEnvironmentVariableA("PATH", searchStr.c_str())) {
                    std::cout << "TinyShell: Successfully removed from PATH:\n-> " << targetPath << "\n";
                } else {
                    std::cerr << "TinyShell: Error: Failed to update PATH environment variable.\n";
                }
            } else {
                std::cerr << "TinyShell: Warning: Path not found in the current environment.\n";
            }
        } else {
            std::cerr << "TinyShell: Error: Could not read current PATH variable.\n";
        }
        return true;
    }

    // 8.2 Export/Set Environment Variable
    else if (args[0] == "export" || args[0] == "set") {
        if (args.size() < 2) {
            std::cerr << "TinyShell: Usage: export VAR=VALUE\n";
            return true;
        }

        // Merge parameters in case value contains spaces
        std::string expr = args[1];
        for (size_t i = 2; i < args.size(); i++) expr += " " + args[i];

        size_t eqPos = expr.find('=');
        if (eqPos == std::string::npos) {
            std::cerr << "TinyShell: Invalid syntax. Use VAR=VALUE\n";
            return true;
        }

        std::string varName = expr.substr(0, eqPos);
        std::string varValue = expr.substr(eqPos + 1);

        // Support automatic macro expansion like %VAR% 
        size_t macroPos = varValue.find("%" + varName + "%");
        if (macroPos != std::string::npos) {
            char currentVal[32767];
            if (GetEnvironmentVariableA(varName.c_str(), currentVal, 32767)) {
                varValue.replace(macroPos, varName.length() + 2, currentVal);
            } else {
                varValue.replace(macroPos, varName.length() + 2, ""); // If variable does not exist yet
            }
        }

        // Apply environment variable update
        if (SetEnvironmentVariableA(varName.c_str(), varValue.c_str())) {
            std::cout << "TinyShell: Environment variable '" << varName << "' updated.\n";
        } else {
            std::cerr << "TinyShell: Failed to set environment variable.\n";
        }
        return true;
    }
    
    // 9. List background processes
    else if (args[0] == "list") {
        if (args.size() > 1) {
            std::cerr << "TinyShell: Error: too many arguments for list command\n";
            return true;    
        }
        ListProcesses();
        return true;
    }
    
    // 10. Kill process command
    else if (args[0] == "kill") {
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for kill command\n";
        }
        else if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing target for kill command. Use <PID> or %<JobID>\n";
        } else {
            KillProcess(args[1]); 
        }
        return true;
    }
    
    // 11. Suspend process command
    else if (args[0] == "stop") {
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for stop command\n";    
        }
        else if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing target for stop command. Use <PID> or %<JobID>\n";
        } 
        else {
            StopProcess(args[1]);
        }
        return true;
    }
    
    // 12. Resume process command
    else if (args[0] == "resume") {
        if (args.size() > 2) {
            std::cerr << "TinyShell: Error: too many arguments for resume command\n";
        }
        else if (args.size() < 2) {
            std::cerr << "TinyShell: Error: Missing target for resume command. Use <PID> or %<JobID>\n";
        } 
        else {
            ResumeProcess(args[1]);
        }
        return true;
    } 

    return false; // Return false if not a built-in command (pass to external process)
}