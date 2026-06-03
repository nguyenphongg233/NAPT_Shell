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
