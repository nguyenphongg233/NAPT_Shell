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
    
    // Sort completions alphabetically
    std::sort(completions.begin(), completions.end());
    
    return completions;
}

// Show tab completion suggestions for ALL commands and auto-fill
std::string ShowCompletionSuggestions(const std::string& currentInput) {
    if (currentInput.empty()) {
        return currentInput;
    }

    std::string prefix = "";
    std::string pathToComplete = currentInput;

    // 1. DYNAMIC PREFIX PARSING
    // Find the last space to separate the command/previous arguments from the current argument
    size_t lastSpace = currentInput.find_last_of(' ');
    if (lastSpace != std::string::npos) {
        prefix = currentInput.substr(0, lastSpace + 1); // e.g., "ls -l " or "cd "
        pathToComplete = currentInput.substr(lastSpace + 1); // e.g., "Doc"
    }

    // Context-aware flag: Only 'cd' strictly requires directories
    bool dirOnly = (prefix == "cd ");

    // 2. PATH EXTRACTION
    size_t lastSlash = pathToComplete.find_last_of("\\/");
    std::string basePath = (lastSlash != std::string::npos) ? pathToComplete.substr(0, lastSlash + 1) : "";
    std::string partial = (lastSlash != std::string::npos) ? pathToComplete.substr(lastSlash + 1) : pathToComplete;
    
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    std::string searchPath = basePath.empty() ? "*" : basePath + "*";
    
    findHandle = FindFirstFileA(searchPath.c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return currentInput;
    }
    
    std::vector<std::string> suggestions;
    
    // 3. CONTEXT-AWARE FILTERING
    do {
        std::string name = findData.cFileName;
        
        // Skip . and ..
        if (name == "." || name == "..") continue;
        
        bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
        
        // If it's the 'cd' command, strictly filter out files
        if (dirOnly && !isDirectory) {
            continue;
        }
        
        // Match partial input
        if (partial.empty() || name.find(partial) == 0) {
            std::string match = basePath + name;
            
            // Add trailing slash for directories to improve UX
            if (isDirectory) {
                match += "\\";
            }
            suggestions.push_back(match);
        }
    } while (FindNextFileA(findHandle, &findData));
    
    FindClose(findHandle);
    
    if (suggestions.empty()) {
        return currentInput;
    }

    // --- LONGEST COMMON PREFIX (LCP) ALGORITHM ---
    std::string lcp = suggestions[0];
    for (size_t i = 1; i < suggestions.size(); ++i) {
        size_t j = 0;
        while (j < lcp.size() && j < suggestions[i].size() && lcp[j] == suggestions[i][j]) {
            j++;
        }
        lcp = lcp.substr(0, j); 
    }

    // --- QUICK AUTO-COMPLETE FOR SINGLE MATCH ---
    // Note: We don't need to manually add "\\" here anymore because 
    // the filtering loop above already added it to directories!

    // Combine into the new completed command string
    std::string newInput = prefix + lcp;

    // --- CONSOLE DISPLAY HANDLING ---
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);

    if (suggestions.size() > 1) {
        std::cout << "\n";
        for (const auto& suggestion : suggestions) {
            std::cout << "  " << suggestion << "\n";
        }
        std::cout << cwd << "> " << newInput;
        std::cout.flush();
    } else {
        std::cout << "\r" << cwd << "> " << newInput;
        std::cout << "          \b\b\b\b\b\b\b\b\b\b"; 
        std::cout.flush();
    }

    return newInput;
}

// List files in directory (for ls command)
void ListDirectory(const std::string& path) {
    WIN32_FIND_DATAA findData;
    HANDLE findHandle;
    std::string searchPath; 
    
    if (path.empty()) {
        searchPath = "*";
    } else {
        DWORD attrs = GetFileAttributesA(path.c_str());
        
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            if (path.back() == '\\' || path.back() == '/') {
                searchPath = path + "*";
            } else {
                searchPath = path + "\\*";
            }
        } else {
            searchPath = path;
        }
    }
    
    findHandle = FindFirstFileA(searchPath.c_str(), &findData);
    
    if (findHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "TinyShell: Cannot access directory '" << path << "'. Error: " << GetLastError() << "\n";
        return;
    }
    
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