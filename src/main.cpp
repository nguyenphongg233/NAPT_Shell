#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <conio.h>
#include "../include/parser.h"
#include "../include/builtin.h"
#include "../include/process_mgr.h"
#include "../include/signal_hnd.h"
#include "../include/executor.h"
#include "../include/completion.h"

int main() {
    SetupSignalHandler();
    std::string input;
    int cursorPos = 0;
    char cwd[MAX_PATH];

    // --- COMMAND HISTORY VARIABLES ---
    std::vector<std::string> history;
    int historyIndex = 0;
    std::string tempInput = ""; // Temporarily store the currently active unexecuted line

    while (true) {
        CleanUpProcesses(); // Clean up dead background processes

        if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
            std::cout << cwd << "> ";
        } else {
            std::cout << "TinyShell> ";
        }
        std::cout.flush();

        input = "";
        cursorPos = 0; 
        historyIndex = history.size(); // Reset history index to the end for a new command entry
        
        while (true) {
            int ch = _getch();  
            
            // 1. HANDLE SPECIAL KEYS (Arrow keys)
            if (ch == 0 || ch == 224) {
                int arrow = _getch(); 
                
                if (arrow == 75) { // LEFT Arrow
                    if (cursorPos > 0) {
                        cursorPos--;
                        std::cout << "\b"; 
                        std::cout.flush(); // Flush screen buffer immediately
                    }
                } 
                else if (arrow == 77) { // RIGHT Arrow
                    if (cursorPos < input.length()) {
                        std::cout << input[cursorPos]; 
                        cursorPos++;
                        std::cout.flush(); 
                    }
                }
                else if (arrow == 72) { // UP Arrow (Browse older history)
                    if (historyIndex > 0) {
                        // Save the current input line if moving away from the active entry for the first time
                        if (historyIndex == history.size()) {
                            tempInput = input;
                        }
                        historyIndex--;
                        
                        // Cleanly wipe the existing line from the screen regardless of the current cursorPos
                        for (int i = 0; i < cursorPos; i++) std::cout << "\b";
                        for (int i = 0; i < input.length(); i++) std::cout << " ";
                        for (int i = 0; i < input.length(); i++) std::cout << "\b";
                        
                        // Fetch and render the historical command
                        input = history[historyIndex];
                        std::cout << input;
                        cursorPos = input.length();
                        std::cout.flush();
                    }
                }
                else if (arrow == 80) { // DOWN Arrow (Browse newer history)
                    if (historyIndex < history.size()) {
                        historyIndex++;
                        
                        // Cleanly wipe the existing line from the screen
                        for (int i = 0; i < cursorPos; i++) std::cout << "\b";
                        for (int i = 0; i < input.length(); i++) std::cout << " ";
                        for (int i = 0; i < input.length(); i++) std::cout << "\b";
                        
                        // Restore either a newer history entry or the saved temporary unexecuted line
                        input = (historyIndex == history.size()) ? tempInput : history[historyIndex];
                        std::cout << input;
                        cursorPos = input.length();
                        std::cout.flush();
                    }
                }
                continue;
            }
            
            // 2. HANDLE TAB KEY (Context-aware Auto-completion)
            else if (ch == '\t') {  
                input = ShowCompletionSuggestions(input);
                cursorPos = input.length(); 
                continue;
            } 
            
            // 3. HANDLE ENTER KEY (Execute command)
            else if (ch == '\r') {  
                std::cout << "\n";
                // Save to history list only if it's not empty and not a duplicate of the immediate last command
                if (!input.empty() && (history.empty() || history.back() != input)) {
                    history.push_back(input);
                }
                break;
            }
            
            // 4. HANDLE BACKSPACE (In-place character deletion)
            else if (ch == '\b') {  
                if (cursorPos > 0) {
                    input.erase(cursorPos - 1, 1);
                    cursorPos--;

                    std::cout << "\b"; 
                    for (int i = cursorPos; i < input.length(); i++) {
                        std::cout << input[i];
                    }
                    std::cout << " \b"; 

                    for (int i = input.length(); i > cursorPos; i--) {
                        std::cout << "\b";
                    }
                    std::cout.flush(); 
                }
                continue;
            } 
            
            // 5. HANDLE PRINTABLE CHARACTERS (Standard text insertion)
            else if (ch >= 32 && ch < 127) {  
                input.insert(cursorPos, 1, (char)ch);
    
                for (int i = cursorPos; i < input.length(); i++) {
                    std::cout << input[i];
                }
                cursorPos++;
                
                for (int i = input.length(); i > cursorPos; i--) {
                    std::cout << "\b";
                }
                std::cout.flush(); 
            }
        }

        // Trim trailing whitespaces before internal parsing
        while (!input.empty() && input.back() == ' ') input.pop_back();
        if (input.empty()) continue;

        // Detect and isolate the background execution token '&'
        bool isBackground = false;
        if (input.back() == '&') {
            isBackground = true;
            input.pop_back(); 
            while (!input.empty() && input.back() == ' ') input.pop_back(); 
        }

        std::vector<std::string> args = ParseInput(input);
        if (args.empty()) continue;

        // Routing engine: Route to internal builtins or fallback to secondary OS loader
        if (!HandleBuiltin(args)) {
            LaunchExternal(input, isBackground);
        }
    }
    return 0;
}