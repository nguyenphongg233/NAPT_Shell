#include "../include/process_mgr.h"
#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>

std::vector<ProcessInfo> bgProcesses;
std::mutex bgMutex;

std::string GetFileName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

void AddBackgroundProcess(DWORD pid, HANDLE hProcess, HANDLE hThread, const std::string& name) {
    std::lock_guard<std::mutex> lock(bgMutex);
    bgProcesses.push_back({pid, hProcess, hThread, name, "Running"});
    // Print the Job ID when a new background process is created
    std::cout << "[Background] Process created with PID: " << pid 
              << " (Job ID: %" << bgProcesses.size() << ")\n";
}

void CleanUpProcesses() {
    std::lock_guard<std::mutex> lock(bgMutex);
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
    CleanUpProcesses();
    std::lock_guard<std::mutex> lock(bgMutex); 
    
    // Header (Added Job ID column)
    std::cout << std::left << std::setw(10) << "Job ID"
              << std::setw(15) << "Process ID" 
              << std::setw(25) << "Command Name" 
              << "Status\n";
    std::cout << std::string(65, '-') << "\n"; // Increase the separator line length to 65

    int jobId = 1;
    for (const auto& p : bgProcesses) {
        // 1. Extract only the file name instead of the full path
        std::string displayName = GetFileName(p.cmdName);
        
        // 2. If the name is too long (> 22 chars), truncate and append "..."
        if (displayName.length() > 22) {
            displayName = displayName.substr(0, 22) + "...";
        }
        
        // Create Job ID display string (e.g., [%1])
        std::string jobStr = "[%" + std::to_string(jobId++) + "]";

        // 3. Print with standard formatting
        std::cout << std::left << std::setw(10) << jobStr
                  << std::setw(15) << p.pid 
                  << std::setw(25) << displayName 
                  << p.status << "\n";
    }
    std::cout << std::string(65, '-') << "\n";
}

// --- HELPER FUNCTION: Convert input string (%ID or PID) to actual PID ---
// Note: This function must only be called when bgMutex is already locked!
DWORD ResolveTargetToPID(const std::string& target) {
    if (target.empty()) return 0;

    // Scenario 1: User enters a Job ID (starts with '%')
    if (target[0] == '%') { 
        try {
            int jobId = std::stoi(target.substr(1));
            if (jobId > 0 && jobId <= (int)bgProcesses.size()) {
                return bgProcesses[jobId - 1].pid; // Map Job ID to actual PID
            }
            std::cerr << "TinyShell: Error: Job ID %" << jobId << " not found.\n";
            return 0;
        } catch (...) {
            std::cerr << "TinyShell: Error: Invalid Job ID format.\n";
            return 0;
        }
    } 
    // Scenario 2: User enters a PID directly
    else { 
        try {
            return std::stoul(target);
        } catch (...) {
            std::cerr << "TinyShell: Error: Invalid format. Use <PID> or %<JobID>.\n";
            return 0;
        }
    }
}

void KillProcess(const std::string& target) {
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return; // Return immediately if target resolution fails

    for (auto it = bgProcesses.begin(); it != bgProcesses.end(); ++it) {
        if (it->pid == pid) {
            if (TerminateProcess(it->hProcess, 0)) {
                std::cout << "Killed process " << pid << "\n";
            } else {
                std::cerr << "Failed to kill process " << pid << "\n";
            }
            CloseHandle(it->hProcess);
            CloseHandle(it->hThread);
            bgProcesses.erase(it);
            return;
        }
    }
    std::cerr << "Process ID " << pid << " not found in background list.\n";
}

void StopProcess(const std::string& target) {
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return;

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

void ResumeProcess(const std::string& target) {
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return;

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