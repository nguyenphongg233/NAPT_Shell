#include "../include/process_mgr.h"
#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <tlhelp32.h>
#include <algorithm>
#include <unordered_map>
#include <thread>
#include <chrono>

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

std::string ToLower(std::string str) {
    for (char& c : str) c = std::tolower(c);
    return str;
}

void CleanUpProcesses() {
    std::lock_guard<std::mutex> lock(bgMutex);
    for (auto it = bgProcesses.begin(); it != bgProcesses.end(); ) {
        DWORD exitCode;
        if (GetExitCodeProcess(it->hProcess, &exitCode) && exitCode != STILL_ACTIVE) {
            
            // --- UWP APP WORKAROUND LOGIC ---
            // 1. Extract base executable name
            std::string baseName = GetFileName(it->cmdName);
            size_t spacePos = baseName.find(' ');
            if (spacePos != std::string::npos) baseName = baseName.substr(0, spacePos);
            if (ToLower(baseName).find(".exe") == std::string::npos) baseName += ".exe";

            std::string searchName = ToLower(baseName);

            static const std::unordered_map<std::string, std::string> uwpAliases = {
                {"calc.exe",         "calculatorapp.exe"},
                {"wt.exe",           "windowsterminal.exe"},
                {"mspaint.exe",      "paintapp.exe"},
                {"pbrush.exe",       "paintapp.exe"},
                {"stikynot.exe",     "microsoft.notes.exe"}
            };

            auto aliasIt = uwpAliases.find(searchName);
            if (aliasIt != uwpAliases.end()) {
                searchName = aliasIt->second; 
            }

            DWORD realPid = 0;
            
            // 2. Scan system for the "Real" process
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe;
                pe.dwSize = sizeof(pe);
                if (Process32First(hSnap, &pe)) {
                    do {
                        if (ToLower(pe.szExeFile) == searchName) {
                            realPid = pe.th32ProcessID;
                            break; // Found the real process!
                        }
                    } while (Process32Next(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }

            // 3. Hijack the new PID if found
            if (realPid != 0 && realPid != it->pid) {
                CloseHandle(it->hProcess); // Close dead stub handles
                CloseHandle(it->hThread);
                
                // Update with new real process info
                it->pid = realPid;
                it->hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, realPid);
                
                // Recover the main thread of the new process
                hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
                if (hSnap != INVALID_HANDLE_VALUE) {
                    THREADENTRY32 te;
                    te.dwSize = sizeof(te);
                    if (Thread32First(hSnap, &te)) {
                        do {
                            if (te.th32OwnerProcessID == realPid) {
                                it->hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                                break;
                            }
                        } while (Thread32Next(hSnap, &te));
                    }
                    CloseHandle(hSnap);
                }
                ++it; // Keep it in the list and move on
            } else {
                // It's truly dead, remove it from list
                CloseHandle(it->hProcess);
                CloseHandle(it->hThread);
                it = bgProcesses.erase(it);
            }
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

    bool found = false;
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            found = true;
            
            // Take a snapshot of all running threads in the entire system
            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        // If this thread belongs to our process -> Suspend it!
                        if (te32.th32OwnerProcessID == pid) {
                            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                            if (hThread != NULL) {
                                SuspendThread(hThread);
                                CloseHandle(hThread);
                            }
                        }
                    } while (Thread32Next(hThreadSnapshot, &te32));
                }
                CloseHandle(hThreadSnapshot);
            }
            
            p.status = "Stopped";
            std::cout << "Stopped process " << pid << " (All threads suspended)\n";
            return;
        }
    }
    if (!found) std::cerr << "Process ID " << pid << " not found.\n";
}

void ResumeProcess(const std::string& target) {
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return;

    bool found = false;
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            found = true;

            // Do the same: Scan all threads and awaken (Resume) them
            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        if (te32.th32OwnerProcessID == pid) {
                            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
                            if (hThread != NULL) {
                                ResumeThread(hThread);
                                CloseHandle(hThread);
                            }
                        }
                    } while (Thread32Next(hThreadSnapshot, &te32));
                }
                CloseHandle(hThreadSnapshot);
            }

            p.status = "Running";
            std::cout << "Resumed process " << pid << " (All threads awakened)\n";
            return;
        }
    }
    if (!found) std::cerr << "Process ID " << pid << " not found.\n";
}

void TerminateAllProcesses() {
    std::lock_guard<std::mutex> lock(bgMutex);
    int count = 0;
    
    for (auto& p : bgProcesses) {
        DWORD exitCode;
        if (GetExitCodeProcess(p.hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            if (TerminateProcess(p.hProcess, 0)) {
                count++;
            }
        }
        CloseHandle(p.hProcess);
        CloseHandle(p.hThread);
    }
    
    bgProcesses.clear(); 
    
    if (count > 0) {
        std::cout << "TinyShell: Terminated " << count << " background process(es) before exiting.\n";
    }
}

void SleepProcessForDuration(const std::string& target, int seconds) {
    if (seconds <= 0) {
        std::cerr << "TinyShell: Error: Duration must be greater than 0 seconds.\n";
        return;
    }

    std::thread([target, seconds]() {
        StopProcess(target);
        
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        
        ResumeProcess(target);
        
    }).detach(); 
}