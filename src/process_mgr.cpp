#include "../include/process_mgr.h"
#include <iostream>
#include <iomanip>

std::vector<ProcessInfo> bgProcesses;

void AddBackgroundProcess(DWORD pid, HANDLE hProcess, HANDLE hThread, const std::string& name) {
    bgProcesses.push_back({pid, hProcess, hThread, name, "Running"});
    std::cout << "[Background] Process created with PID: " << pid << "\n";
}

void CleanUpProcesses() {
    for (auto it = bgProcesses.begin(); it != bgProcesses.end(); ) {
        DWORD exitCode;
        if (GetExitCodeProcess(it->hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            CloseHandle(it->hProcess);
            CloseHandle(it->hThread);
            it = bgProcesses.erase(it);
        } else {
            ++it;
        }
    }
}

void ListProcesses() {
    CleanUpProcesses(); // Dọn rác trước khi in
    std::cout << std::left << std::setw(15) << "Process ID" 
              << std::setw(25) << "Command Name" 
              << "Status\n";
    std::cout << std::string(50, '-') << "\n";
    for (const auto& p : bgProcesses) {
        std::cout << std::left << std::setw(15) << p.pid 
                  << std::setw(25) << p.cmdName 
                  << p.status << "\n";
    }
}

void KillProcess(DWORD pid) {
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            if (TerminateProcess(p.hProcess, 0)) {
                std::cout << "Killed process " << pid << "\n";
            } else {
                std::cerr << "Failed to kill process " << pid << "\n";
            }
            return;
        }
    }
    std::cerr << "Process ID " << pid << " not found in background list.\n";
}

void StopProcess(DWORD pid) {
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            if (SuspendThread(p.hThread) != (DWORD)-1) {
                p.status = "Stopped";
                std::cout << "Stopped process " << pid << "\n";
            }
            return;
        }
    }
    std::cerr << "Process ID " << pid << " not found.\n";
}

void ResumeProcess(DWORD pid) {
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            if (ResumeThread(p.hThread) != (DWORD)-1) {
                p.status = "Running";
                std::cout << "Resumed process " << pid << "\n";
            }
            return;
        }
    }
    std::cerr << "Process ID " << pid << " not found.\n";
}