#include "../include/process_mgr.h"
#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <tlhelp32.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <thread>
#include <chrono>

std::vector<ProcessInfo> bgProcesses;
std::mutex bgMutex;

// Extract the base file name from a full path
std::string GetFileName(const std::string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

// Add a new background process to the management list
void AddBackgroundProcess(DWORD pid, HANDLE hProcess, HANDLE hThread, const std::string& name) {
    std::lock_guard<std::mutex> lock(bgMutex);
    bgProcesses.push_back({pid, hProcess, hThread, name, "Running"});
    // Print the Job ID when a new background process is created
    std::cout << "[Background] Process created with PID: " << pid 
              << " (Job ID: %" << bgProcesses.size() << ")\n";
}

// Convert a string to lowercase for case-insensitive comparison
std::string ToLower(std::string str) {
    for (char& c : str) c = std::tolower(c);
    return str;
}

// Clean up dead processes and handle UWP App PID hijacking
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

            // Map common UWP command names to their actual background executable names
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
            
            // 2. Scan system for the actual running process matching the alias
            HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (hSnap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe;
                pe.dwSize = sizeof(pe);
                if (Process32First(hSnap, &pe)) {
                    do {
                        if (ToLower(pe.szExeFile) == searchName) {
                            realPid = pe.th32ProcessID;
                            break; // Found the real UWP process
                        }
                    } while (Process32Next(hSnap, &pe));
                }
                CloseHandle(hSnap);
            }

            // 3. Hijack the new PID if found (Replace the dead stub with the real app)
            if (realPid != 0 && realPid != it->pid) {
                CloseHandle(it->hProcess); // Close dead stub handles
                CloseHandle(it->hThread);
                
                // Update with new real process info
                it->pid = realPid;
                it->hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE, FALSE, realPid);
                
                // Recover the main thread of the new process for Suspend/Resume capabilities
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
                // It's truly dead, remove it from the tracking list entirely
                CloseHandle(it->hProcess);
                CloseHandle(it->hThread);
                it = bgProcesses.erase(it);
            }
        } else {
            ++it;
        }
    }
}

// Display all active background processes
void ListProcesses() {
    CleanUpProcesses();
    std::lock_guard<std::mutex> lock(bgMutex); 
    
    // Display Header
    std::cout << std::left << std::setw(10) << "Job ID"
              << std::setw(15) << "Process ID" 
              << std::setw(25) << "Command Name" 
              << "Status\n";
    std::cout << std::string(65, '-') << "\n"; 

    int jobId = 1;
    for (const auto& p : bgProcesses) {
        // Extract only the file name instead of the full path for cleaner display
        std::string displayName = GetFileName(p.cmdName);
        
        // Truncate long names to maintain table alignment
        if (displayName.length() > 22) {
            displayName = displayName.substr(0, 22) + "...";
        }
        
        // Format Job ID string (e.g., [%1])
        std::string jobStr = "[%" + std::to_string(jobId++) + "]";

        std::cout << std::left << std::setw(10) << jobStr
                  << std::setw(15) << p.pid 
                  << std::setw(25) << displayName 
                  << p.status << "\n";
    }
    std::cout << std::string(65, '-') << "\n";
}

// --- HELPER FUNCTION: Convert input string (%ID or PID) to actual PID ---
// Note: This function assumes bgMutex is already locked by the caller!
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

// Gather all child PIDs belonging to a parent PID (Process Tree)
std::vector<DWORD> GetProcessTree(DWORD parentPid) {
    std::vector<DWORD> pids = { parentPid };
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hProcessSnap, &pe32)) {
            do {
                if (pe32.th32ParentProcessID == parentPid) {
                    pids.push_back(pe32.th32ProcessID);
                }
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);
    }
    return pids;
}

// Terminate a process and all its children
void KillProcess(const std::string& target) {
    CleanUpProcesses(); // Ensure UWP states are updated before targeting
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return; 

    for (auto it = bgProcesses.begin(); it != bgProcesses.end(); ++it) {
        if (it->pid == pid) {
            // Get the entire process tree to prevent ghost/zombie windows
            std::vector<DWORD> treePids = GetProcessTree(pid);
            
            // Terminate all processes in the tree
            for (DWORD targetPid : treePids) {
                HANDLE hTarget = OpenProcess(PROCESS_TERMINATE, FALSE, targetPid);
                if (hTarget != NULL) {
                    TerminateProcess(hTarget, 0);
                    CloseHandle(hTarget);
                }
            }
            
            std::cout << "Killed process tree for PID " << pid << "\n";
            CloseHandle(it->hProcess);
            CloseHandle(it->hThread);
            bgProcesses.erase(it);
            return;
        }
    }
    std::cerr << "Process ID " << pid << " not found in background list.\n";
}

// Suspend a process and all its children
void StopProcess(const std::string& target) {
    CleanUpProcesses();
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return;

    bool found = false;
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            found = true;
            
            // Get process tree
            std::vector<DWORD> treePids = GetProcessTree(pid);
            
            // Take a snapshot of all threads in the system
            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        // If thread belongs to our target process tree, suspend it
                        if (std::find(treePids.begin(), treePids.end(), te32.th32OwnerProcessID) != treePids.end()) {
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
            std::cout << "Stopped process " << pid << " (All tree threads suspended)\n";
            return;
        }
    }
    if (!found) std::cerr << "Process ID " << pid << " not found.\n";
}

// Resume a suspended process and all its children
void ResumeProcess(const std::string& target) {
    CleanUpProcesses();
    std::lock_guard<std::mutex> lock(bgMutex);
    DWORD pid = ResolveTargetToPID(target);
    if (pid == 0) return;

    bool found = false;
    for (auto& p : bgProcesses) {
        if (p.pid == pid) {
            found = true;

            std::vector<DWORD> treePids = GetProcessTree(pid);

            HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te32;
                te32.dwSize = sizeof(THREADENTRY32);

                if (Thread32First(hThreadSnapshot, &te32)) {
                    do {
                        // Awaken all threads belonging to the process tree
                        if (std::find(treePids.begin(), treePids.end(), te32.th32OwnerProcessID) != treePids.end()) {
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
            std::cout << "Resumed process " << pid << " (All tree threads awakened)\n";
            return;
        }
    }
    if (!found) std::cerr << "Process ID " << pid << " not found.\n";
}

// Graceful Shutdown: Terminate all background process trees before exiting shell
void TerminateAllProcesses() {
    CleanUpProcesses(); // Crucial: Update PIDs for UWP apps before mass termination
    std::lock_guard<std::mutex> lock(bgMutex);
    int count = 0;
    
    for (auto& p : bgProcesses) {
        std::vector<DWORD> treePids = GetProcessTree(p.pid);
        
        for (DWORD targetPid : treePids) {
            HANDLE hTarget = OpenProcess(PROCESS_TERMINATE, FALSE, targetPid);
            if (hTarget != NULL) {
                TerminateProcess(hTarget, 0);
                CloseHandle(hTarget);
            }
        }
        
        CloseHandle(p.hProcess);
        CloseHandle(p.hThread);
        count++;
    }
    
    bgProcesses.clear(); 
    
    if (count > 0) {
        std::cout << "TinyShell: Terminated " << count << " background process tree(s) before exiting.\n";
    }
}

// Asynchronous Sleep: Suspends a process, waits without blocking the shell, then resumes
void SleepProcessForDuration(const std::string& target, int seconds) {
    if (seconds <= 0) {
        std::cerr << "TinyShell: Error: Duration must be greater than 0 seconds.\n";
        return;
    }

    // Launch an independent background thread
    std::thread([target, seconds]() {
        // 1. Suspend the process
        StopProcess(target);
        
        // 2. Wait for the specified duration (Does not block the main shell input)
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        
        // 3. Awaken the process
        ResumeProcess(target);
        
    }).detach(); // Detach allows the thread to run and clean itself up independently
}