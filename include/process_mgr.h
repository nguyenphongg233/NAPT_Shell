#ifndef PROCESS_MGR_H
#define PROCESS_MGR_H

#include <windows.h>
#include <string>
#include <vector>

// Cấu trúc lưu trữ thông tin tiến trình con
struct ProcessInfo {
    DWORD pid;
    HANDLE hProcess;
    HANDLE hThread;
    std::string cmdName;
    std::string status; // "Running" hoặc "Stopped"
};

void AddBackgroundProcess(DWORD pid, HANDLE hProcess, HANDLE hThread, const std::string& name);
void ListProcesses();
void KillProcess(DWORD pid);
void StopProcess(DWORD pid);
void ResumeProcess(DWORD pid);
void CleanUpProcesses(); // Xóa các process đã kết thúc khỏi danh sách

#endif // PROCESS_MGR_H