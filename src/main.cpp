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
    char cwd[MAX_PATH];

    while (true) {
        CleanUpProcesses(); // Clean up dead background processes

        if (GetCurrentDirectoryA(MAX_PATH, cwd)) {
            std::cout << cwd << "> ";
        } else {
            std::cout << "TinyShell> ";
        }
        std::cout.flush();

        // Read input character by character to handle Tab key
        input = "";
        while (true) {
            int ch = _getch();  // Read single character without echo
            
            if (ch == 0 || ch == 0xE0) {  // Special key prefix (arrow keys, function keys)
                _getch(); // Consume the second byte of the special key sequence
                continue;
            }
            else if (ch == '\t') {  // Tab key
                // Show completion suggestions for cd command
                if (input.find("cd ") == 0) {
                    ShowCompletionSuggestions(input);
                }
                continue;
            } 
            else if (ch == '\r') {  // Enter key
                std::cout << "\n";
                break;
            } 
            else if (ch == '\b') {  // Backspace
                if (!input.empty()) {
                    input.pop_back();
                    std::cout << "\b \b";
                    std::cout.flush();
                }
                continue;
            } 
            else if (ch >= 32 && ch < 127) {  // Printable characters
                input += static_cast<char>(ch);
                std::cout << static_cast<char>(ch);
                std::cout.flush();
            }
        }

        if (input.empty()) continue;

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