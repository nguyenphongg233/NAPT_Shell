#ifndef PROCESS_MGR_H
#define PROCESS_MGR_H

#include <windows.h>
#include <string>
#include <vector>

// Structure to store child process information
struct ProcessInfo {
    DWORD pid;
    HANDLE hProcess;
    HANDLE hThread;
    std::string cmdName;
    std::string status; // "Running" or "Stopped"
};

void AddBackgroundProcess(DWORD pid, HANDLE hProcess, HANDLE hThread, const std::string& name);
void ListProcesses();
void KillProcess(const std::string& target);
void StopProcess(const std::string& target);
void ResumeProcess(const std::string& target);
void CleanUpProcesses(); // Remove terminated processes from list

#endif // PROCESS_MGR_H