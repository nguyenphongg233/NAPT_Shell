#include<bits/stdc++.h>
#include<windows.h>
#pragma comment(lib, "kernel32.lib")

using namespace std;

class TinyShell{
private: 
    struct ProcessInfo {
        DWORD dwProcessId;    // Process ID
        HANDLE hProcess;      // Handle to Kill/Stop process
        HANDLE hThread;       // Handle to Resume/Suspend thread
        string cmdName;       // Command name (e.g., ping)
        bool isRunning;       // Running status
    };

    vector<ProcessInfo> processList; // List of running processes
    vector<string> commandHistory; // Command history
    vector<string> pathList; // List of added PATH directories

    vector<string> split(const string& str) {
        vector<string> tokens;
        stringstream ss(str);
        string words;
        while(ss >> words){
            if(words[0] == '"'){
                string temp;
                while(ss >> temp){
                    words += " " + temp;
                    if(temp.back() == '"') break;
                }
            }
            tokens.push_back(words);
        }
        return tokens;
    }

    struct Command{
        string name;
        vector<string> args;
    };

    
    void help(const vector<string> &args){
        cout << "Available commands:\n";
        cout << "help - Show this help message\n";
        cout << "exit - Exit the shell\n";
        cout << "cls - Clear the console\n";
        cout << "path - Show current PATH\n";
        cout << "addpath [new_path] - Add a new path to PATH\n";
        cout << "history - Show command history\n";
        cout << "fg [command] - Run command in foreground (waits for completion)\n";
        cout << "bg [command] - Run command in background (doesn't wait)\n";
        cout << "list - List all background processes\n";
        cout << "kill <PID> - Terminate a process by ID\n";
        cout << "stop <PID> - Suspend a process by ID\n";
        cout << "resume <PID> - Resume a suspended process by ID\n";
        cout << "Other commands will be executed as system commands if found in PATH.\n";


    }
    void exit(const vector<string> &args){
        cout << "Exiting TinyShell...\n";
        for(const auto &proc : processList){
            if(proc.isRunning){
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.dwProcessId);
                if(hProcess != NULL){
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                    cout << "Terminated process " << proc.dwProcessId << "\n";
                }
            }
        }
        ExitProcess(0);
    }
    void cls(const vector<string> &args){
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD count;
        DWORD cellCount;
        COORD homeCoords = { 0, 0 };

        if (hConsole == INVALID_HANDLE_VALUE) return;

        // 1. Get current console screen buffer info (size, cursor position...)
        if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;

        // Calculate total cells to clear (Width * Height)
        cellCount = csbi.dwSize.X * csbi.dwSize.Y;

        // 2. Overwrite entire screen with blank spaces
        if (!FillConsoleOutputCharacter(
            hConsole,
            (TCHAR) ' ',     // Character to write (space)
            cellCount,       // Number of cells to write
            homeCoords,      // Starting position (0, 0)
            &count           // Variable to receive actual number of chars written
        )) return;

        // 3. (Optional) Reset color attributes (avoid old background colors)
        if (!FillConsoleOutputAttribute(
            hConsole,
            csbi.wAttributes,
            cellCount,
            homeCoords,
            &count
        )) return;

        // 4. Move cursor to top-left corner (0, 0)
        SetConsoleCursorPosition(hConsole, homeCoords);
    }
    void path(const vector<string> &args){
        // cout << "Current PATH:\n";
        // for(const auto& p : pathList) cout << p << "\n";
        char buffer[4096];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        if(size > 0 && size < sizeof(buffer)){
            cout << "Current PATH:\n" << string(buffer) << "\n";
        } else {
            cout << "Failed to get PATH variable.\n";
        }
    }
    void addPath(const vector<string> &args){
        if(args.empty()){
            cout << "Usage: addpath [new_path]\n";
            return;
        }
        string newPath = args[0];
        char buffer[4096];
        DWORD size = GetEnvironmentVariableA("PATH", buffer, sizeof(buffer));
        string newPathStr = (size > 0 && size < sizeof(buffer)) ? string(buffer) + ";" + newPath : newPath;
        if(SetEnvironmentVariableA("PATH", newPathStr.c_str())){
            cout << "Added to PATH: " << newPath << "\n";
        } else {
            cout << "Failed to add to PATH.\n";
        }
    }
    void showHistory(const vector<string> &args){
        cout << "Command History:\n";
        for(const auto& cmd : commandHistory) cout << cmd << "\n";
    }

    // Execute command in foreground - creates new window, waits until process ends
    void Foreground(const string& fullCmd){
        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        
        // Create process with command in a new window
        if (!CreateProcessA(
            NULL,
            (LPSTR)fullCmd.c_str(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi
        )) {
            cout << "Failed to create process\n";
            return;
        }
        
        // Store process info
        ProcessInfo procInfo;
        procInfo.dwProcessId = pi.dwProcessId;
        procInfo.hProcess = pi.hProcess;
        procInfo.hThread = pi.hThread;
        procInfo.cmdName = fullCmd;
        procInfo.isRunning = true;
        processList.push_back(procInfo);
        
        // Wait for process to complete (foreground execution)
        WaitForSingleObject(pi.hProcess, INFINITE);
        
        // Clean up - mark as not running
        for(auto &proc : processList){
            if(proc.dwProcessId == pi.dwProcessId){
                proc.isRunning = false;
                CloseHandle(proc.hProcess);
                CloseHandle(proc.hThread);
                break;
            }
        }
    }
    
    // Execute command in background - doesn't wait, stores info in vector
    void Background(const string& fullCmd){
        STARTUPINFOA si = {};
        PROCESS_INFORMATION pi = {};
        si.cb = sizeof(si);
        
        // Create process with command
        if (!CreateProcessA(
            NULL,
            (LPSTR)fullCmd.c_str(),
            NULL,
            NULL,
            FALSE,
            CREATE_NEW_CONSOLE,
            NULL,
            NULL,
            &si,
            &pi
        )) {
            cout << "Failed to create background process\n";
            return;
        }
        
        // Store process info
        ProcessInfo procInfo;
        procInfo.dwProcessId = pi.dwProcessId;
        procInfo.hProcess = pi.hProcess;
        procInfo.hThread = pi.hThread;
        procInfo.cmdName = fullCmd;
        procInfo.isRunning = true;
        processList.push_back(procInfo);
        
        cout << "Background process started (PID: " << pi.dwProcessId << ")\n";
    }
    
    // List all background processes
    void listProcesses(const vector<string> &args){
        if(processList.empty()){
            cout << "No background processes\n";
            return;
        }
        cout << "Background Processes:\n";
        cout << "PID\t\tCommand\n";
        cout << "---\t\t-------\n";
        for(const auto& proc : processList){
            cout << proc.dwProcessId << "\t\t" << proc.cmdName << "\n";
        }
    }
    
    // Kill a process by ID
    void killProcess(const vector<string> &args){

        if(args.empty()){
            cout << "Usage: kill <PID>\n";
            return;
        }
        if(args[0] == "all"){
            for(auto &proc : processList){
                if(proc.isRunning){
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, proc.dwProcessId);
                    if(hProcess != NULL){
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                        cout << "Process " << proc.dwProcessId << " terminated\n";
                    }
                }
            }
            processList.clear();
            return;
        }
        DWORD pid = stoul(args[0]);
        
        // Find and remove from list
        for(auto it = processList.begin(); it != processList.end(); it++){
            if(it->dwProcessId == pid){
                // Open process and terminate
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if(hProcess != NULL){
                    if(TerminateProcess(hProcess, 0)){
                        cout << "Process " << pid << " terminated\n";
                    } else {
                        cout << "Failed to terminate process " << pid << "\n";
                    }
                    CloseHandle(hProcess);
                }
                processList.erase(it);
                return;
            }
        }
        cout << "Process " << pid << " not found\n";
    }
    
    // Suspend/Stop a process thread by ID
    void stopProcess(const vector<string> &args){
        if(args.empty()){
            cout << "Usage: stop <PID>\n";
            return;
        }
        DWORD pid = stoul(args[0]);
        
        for(auto &proc : processList){
            if(proc.dwProcessId == pid){
                if(SuspendThread(proc.hThread) != (DWORD)-1){
                    cout << "Process " << pid << " suspended\n";
                } else {
                    cout << "Failed to suspend process " << pid << "\n";
                }
                return;
            }
        }
        cout << "Process " << pid << " not found\n";
    }
    
    // Resume a suspended process thread by ID
    void resumeProcess(const vector<string> &args){
        if(args.empty()){
            cout << "Usage: resume <PID>\n";
            return;
        }
        DWORD pid = stoul(args[0]);
        
        for(auto &proc : processList){
            if(proc.dwProcessId == pid){
                if(ResumeThread(proc.hThread) != (DWORD)-1){
                    cout << "Process " << pid << " resumed\n";
                } else {
                    cout << "Failed to resume process " << pid << "\n";
                }
                return;
            }
        }
        cout << "Process " << pid << " not found\n";
    }
    
    // Execute batch file (.bat) using cmd.exe
    void ExecuteBat(const string& batFilePath){
        string cmdLine = "cmd.exe /c " + batFilePath;
        Foreground(cmdLine);
    }
    void runCommand(const Command& cmd){
        if (cmd.name == "help") help(cmd.args);
        else if (cmd.name == "exit") exit(cmd.args);
        else if (cmd.name == "cls") cls(cmd.args);
        else if (cmd.name == "path") path(cmd.args);
        else if (cmd.name == "addpath") addPath(cmd.args);
        else if (cmd.name == "history") showHistory(cmd.args);
        else if (cmd.name == "fg" || cmd.name == "foreground") {
            if(!cmd.args.empty()) Foreground(cmd.args[0]);
        }
        else if (cmd.name == "bg" || cmd.name == "background") {
            if(!cmd.args.empty()) Background(cmd.args[0]);
        }
        else if (cmd.name == "bat") {
            if(!cmd.args.empty()) ExecuteBat(cmd.args[0]);
        }
        else if (cmd.name == "list") listProcesses(cmd.args);
        else if (cmd.name == "kill") killProcess(cmd.args);
        else if (cmd.name == "stop") stopProcess(cmd.args);
        else if (cmd.name == "resume") resumeProcess(cmd.args);
        else cout << "Unknown command: " << cmd.name << "\n";
        commandHistory.push_back(cmd.name);
    }
    

public:
    void run(){
        while(true){
            cout << "~ ";
            string cmd;
            getline(cin, cmd);
            vector<string> tokens = split(cmd); 
            if(tokens.empty()) continue;
            for(auto &c: tokens[0]) c = tolower(c);
            cout << "- ";
            Command command{tokens[0], vector<string>(tokens.begin() + 1, tokens.end())};
            runCommand(command);
        }
    }
};

signed main(){
    TinyShell shell;
    shell.run();
}
