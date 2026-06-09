#include "../include/executor.h"
#include "../include/process_mgr.h"
#include "../include/signal_hnd.h"
#include <iostream>
#include <windows.h>

void LaunchExternal(std::string cmd, bool background) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Handle .bat files: check if the first token ends with .bat (case-insensitive)
    {
        std::string firstToken = cmd.substr(0, cmd.find(' '));
        std::string lowerToken = firstToken;
        for (char& c : lowerToken) c = (char)tolower((unsigned char)c);
        if (lowerToken.size() >= 4 && lowerToken.substr(lowerToken.size() - 4) == ".bat") {
            cmd = "cmd.exe /c " + cmd;
        }
    }

    char* writableCmd = new char[cmd.size() + 1];
    std::copy(cmd.begin(), cmd.end(), writableCmd);
    writableCmd[cmd.size()] = '\0';

    // Use CREATE_NEW_CONSOLE flag for background processes to open in new window
    DWORD dwCreationFlags = background ? CREATE_NEW_CONSOLE : 0;
    
    if (CreateProcessA(NULL, writableCmd, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi)) {
        if (background) {
            AddBackgroundProcess(pi.dwProcessId, pi.hProcess, pi.hThread, cmd);
            std::cout << "TinyShell: Started background process (PID: " << pi.dwProcessId << ") in new window\n";
        } else {
            // Assign PID to global variable to handle CTRL+C
            g_hForegroundProcess = pi.hProcess;
            WaitForSingleObject(pi.hProcess, INFINITE);
            g_hForegroundProcess = NULL; // Clean up after execution
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    } else {
        std::cerr << "Command not found or execution failed.\n";
    }
    delete[] writableCmd;
}

void ExecutePipeline(std::string cmd1, std::string cmd2, bool isBackground) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE; // Allow inheritance of all handles
    sa.lpSecurityDescriptor = NULL;

    HANDLE hReadPipe, hWritePipe;
    
    // 1. CREATE THE PIPE
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        std::cerr << "TinyShell: Error creating pipe.\n";
        return;
    }

    STARTUPINFOA si1, si2;
    PROCESS_INFORMATION pi1, pi2;

    ZeroMemory(&si1, sizeof(si1));  si1.cb = sizeof(si1);
    ZeroMemory(&pi1, sizeof(pi1));
    ZeroMemory(&si2, sizeof(si2));  si2.cb = sizeof(si2);
    ZeroMemory(&pi2, sizeof(pi2));

    // 2. CONFIGURE & RUN PROCESS 1 (Left side of |)
    si1.hStdOutput = hWritePipe;
    si1.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si1.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si1.dwFlags |= STARTF_USESTDHANDLES;

    // Safely convert string for CreateProcess
    std::vector<char> cmd1Buf(cmd1.begin(), cmd1.end()); cmd1Buf.push_back('\0');
    bool success1 = CreateProcessA(NULL, cmd1Buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si1, &pi1);

    // =========================================================================
    // CRUCIAL STEP: The parent shell MUST close the write pipe immediately after 
    // creating process 1. This ensures process 2 does not inherit this pipe 
    // and prevents deadlocks.
    CloseHandle(hWritePipe);
    // =========================================================================

    // 3. CONFIGURE & RUN PROCESS 2 (Right side of |)
    si2.hStdInput = hReadPipe;
    si2.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si2.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    si2.dwFlags |= STARTF_USESTDHANDLES;

    std::vector<char> cmd2Buf(cmd2.begin(), cmd2.end()); cmd2Buf.push_back('\0');
    bool success2 = CreateProcessA(NULL, cmd2Buf.data(), NULL, NULL, TRUE, 0, NULL, NULL, &si2, &pi2);

    // Parent shell closes the read pipe (already handed over to process 2)
    CloseHandle(hReadPipe);

    if (!success1 || !success2) {
        std::cerr << "TinyShell: Error executing pipeline. Check if commands exist.\n";
    }

    // 4. PROCESS MANAGEMENT
    if (success1 && success2) {
        if (!isBackground) {
            // Wait for both processes to finish
            HANDLE handles[2] = {pi1.hProcess, pi2.hProcess};
            WaitForMultipleObjects(2, handles, TRUE, INFINITE);
        } else {
            AddBackgroundProcess(pi2.dwProcessId, pi2.hProcess, pi2.hThread, cmd1 + " | " + cmd2);
        }
    }

    // Clean up handles
    if (!isBackground) {
        if (success1) { CloseHandle(pi1.hProcess); CloseHandle(pi1.hThread); }
        if (success2) { CloseHandle(pi2.hProcess); CloseHandle(pi2.hThread); }
    } else {
        if (success1) { CloseHandle(pi1.hProcess); CloseHandle(pi1.hThread); }
        // pi2 is kept and managed by AddBackgroundProcess
    }
}

void ExecuteWithInputRedirection(std::string cmd, std::string inFile, bool background) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; // Must allow inheritance so child process can read from file
    sa.lpSecurityDescriptor = NULL;

    // Open the existing input file
    HANDLE hFile = CreateFileA(inFile.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "TinyShell: Error: Cannot open input file '" << inFile << "'. Ensure the file exists.\n";
        return;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Redirect Standard Input to the file handle
    si.hStdInput = hFile;
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE); // Keep output on console
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);   // Keep errors on console
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::vector<char> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back('\0');
    
    DWORD dwCreationFlags = background ? CREATE_NEW_CONSOLE : 0;

    bool success = CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE, dwCreationFlags, NULL, NULL, &si, &pi);

    // Parent shell must close its handle to the file immediately after creating process
    CloseHandle(hFile);

    if (success) {
        if (background) {
            AddBackgroundProcess(pi.dwProcessId, pi.hProcess, pi.hThread, cmd + " < " + inFile);
            std::cout << "TinyShell: Started background process (PID: " << pi.dwProcessId << ")\n";
        } else {
            g_hForegroundProcess = pi.hProcess;
            WaitForSingleObject(pi.hProcess, INFINITE);
            g_hForegroundProcess = NULL;
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    } else {
        std::cerr << "Command not found or execution failed.\n";
    }
}

void ExecuteWithOutputRedirection(std::string cmd, std::string outFile, bool background) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE; // Must allow inheritance so child process can write to file
    sa.lpSecurityDescriptor = NULL;

    // Create or open the output file
    HANDLE hFile = CreateFileA(outFile.c_str(), GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "TinyShell: Error: Cannot open or create output file.\n";
        return;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Redirect Standard Output to the file handle
    si.hStdOutput = hFile;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE); // Keep errors on console
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE); // Keep input from keyboard
    si.dwFlags |= STARTF_USESTDHANDLES;

    std::vector<char> cmdBuf(cmd.begin(), cmd.end()); cmdBuf.push_back('\0');
    
    DWORD dwCreationFlags = background ? CREATE_NEW_CONSOLE : 0;

    bool success = CreateProcessA(NULL, cmdBuf.data(), NULL, NULL, TRUE, dwCreationFlags, NULL, NULL, &si, &pi);

    // Parent shell must close its handle to the file immediately after creating process
    CloseHandle(hFile);

    if (success) {
        if (background) {
            AddBackgroundProcess(pi.dwProcessId, pi.hProcess, pi.hThread, cmd + " > " + outFile);
            std::cout << "TinyShell: Started background process (PID: " << pi.dwProcessId << ")\n";
        } else {
            g_hForegroundProcess = pi.hProcess;
            WaitForSingleObject(pi.hProcess, INFINITE);
            g_hForegroundProcess = NULL;
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    } else {
        std::cerr << "Command not found or execution failed.\n";
    }
}