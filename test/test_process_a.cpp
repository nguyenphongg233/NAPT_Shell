#include <iostream>
#include <windows.h>
#include <ctime>

int main() {
    std::cout << "========== Test Process A ==========" << std::endl;
    std::cout << "Process ID: " << GetCurrentProcessId() << std::endl;
    time_t now = time(NULL);
    std::cout << "Started at: " << ctime(&now);
    std::cout << "This is a test process running in foreground mode." << std::endl;
    std::cout << "Performing some operations...\n" << std::endl;
    
    for (int i = 1; i <= 100; i++) {
        std::cout << "  Step " << i << " completed." << std::endl;
        Sleep(500);  // Wait 500ms
    }
    
    std::cout << "\nTest Process A completed successfully!" << std::endl;
    std::cout << "===================================" << std::endl;
    
    return 0;
}
