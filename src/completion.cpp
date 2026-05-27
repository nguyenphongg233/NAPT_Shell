#include "../include/completion.h"
#include <windows.h>
#include <iostream>
#include <algorithm>
#include <sstream>

// List files and directories matching a partial path
std::vector<std::string> GetCompletions(const std::string& partialPath) {
    std::vector<std::string> completions;
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    
    // If partial path is empty, search in current directory
    std::string searchPath = partialPath.empty() ? "*" : partialPath + "*";
    
    findHandle = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        return completions;
    }
    
    do {
        // Skip . and .. directories
        if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
            std::string name = findData.cFileName;
            // Add trailing backslash for directories
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                name += "\\";
            }
            completions.push_back(name);
        }
    } while (FindNextFileA(findHandle, &findData));
    
    FindClose(findHandle);
    
    // Sort completions
    std::sort(completions.begin(), completions.end());
    
    return completions;
}

// Show tab completion suggestions for cd command
void ShowCompletionSuggestions(const std::string& currentInput) {
    // Extract the partial path from "cd <path>"
    std::string path = currentInput;
    
    // Remove "cd " prefix
    if (path.find("cd ") == 0) {
        path = path.substr(3);
    }
    
    // Get last path component for matching
    size_t lastSlash = path.find_last_of("\\/");
    std::string basePath = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";
    std::string partial = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    
    // Find matching directories
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    std::string searchPath = basePath.empty() ? "*" : basePath + "*";
    
    findHandle = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        return;
    }
    
    std::vector<std::string> suggestions;
    
    do {
        std::string name = findData.cFileName;
        // Only show directories
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(name.c_str(), ".") != 0 && strcmp(name.c_str(), "..") != 0) {
            // Match partial input
            if (partial.empty() || name.find(partial) == 0) {
                suggestions.push_back(basePath + name);
            }
        }
    } while (FindNextFileA(findHandle, &findData));
    
    FindClose(findHandle);
    
    // Show suggestions
    if (!suggestions.empty()) {
        std::cout << "\n";
        for (const auto& suggestion : suggestions) {
            std::cout << "  " << suggestion << "\n";
        }
        // Reprint prompt and current input
        char cwd[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
            std::cout << cwd << "> " << currentInput;
            std::cout.flush();
        }
    }
}

// List files in directory (for ls command)
void ListDirectory(const std::string& path) {
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    
    std::string searchPath = path.empty() ? "*" : path + "\\*";
    
    findHandle = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "TinyShell: Cannot access directory '" << path << "'. Error: " << GetLastError() << "\n";
        return;
    }
    
    std::cout << "\n";
    std::cout << "Directory of " << (path.empty() ? "." : path) << ":\n\n";
    
    do {
        std::string name = findData.cFileName;
        
        // Show directory indicator
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "[DIR]  " << name << "\n";
        } else {
            std::cout << "       " << name << "\n";
        }
    } while (FindNextFileA(findHandle, &findData));
    
    FindClose(findHandle);
    std::cout << "\n";
}
