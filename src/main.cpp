#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include "../include/parser.h"
#include "../include/builtin.h"
#include "../include/process_mgr.h"
#include "../include/signal_hnd.h"

void LaunchExternal(std::string cmd, bool background) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Xử lý riêng cho file *.bat
    if (cmd.find(".bat") != std::string::npos) {
        cmd = "cmd.exe /c " + cmd;
    }

    char* writableCmd = new char[cmd.size() + 1];
    std::copy(cmd.begin(), cmd.end(), writableCmd);
    writableCmd[cmd.size()] = '\0';

    if (CreateProcessA(NULL, writableCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        if (background) {
            AddBackgroundProcess(pi.dwProcessId, pi.hProcess, pi.hThread, cmd);
        } else {
            // Gán PID cho biến toàn cục để hứng ngắt CTRL+C
            g_hForegroundProcess = pi.hProcess;
            WaitForSingleObject(pi.hProcess, INFINITE);
            g_hForegroundProcess = NULL; // Dọn dẹp sau khi chạy xong
            
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    } else {
        std::cerr << "Command not found or execution failed.\n";
    }
    delete[] writableCmd;
}

int main() {
    SetupSignalHandler();
    std::string input;
    char cwd[MAX_PATH];

    while (true) {
        CleanUpProcesses(); // Dọn dẹp tiến trình ngầm đã chết

        if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
            std::cout << cwd << "> ";
        } else {
            std::cout << "TinyShell> ";
        }

        if (!std::getline(std::cin, input) || input.empty()) continue;

        bool isBackground = false;
        if (input.back() == '&') {
            isBackground = true;
            input.pop_back(); 
            while (!input.empty() && input.back() == ' ') input.pop_back(); // Trim space
        }

        std::vector<std::string> args = ParseInput(input);
        if (args.empty()) continue;

        if (!HandleBuiltin(args)) {
            LaunchExternal(input, isBackground);
        }
    }
    return 0;
}