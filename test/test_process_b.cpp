#include <iostream>
#include <cstdlib>
#include <ctime>
#include <windows.h>

int main() {
    std::cout << "========== Test Process B ==========" << std::endl;
    std::cout << "Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "This background test process will run for 15 seconds.\n" << std::endl;
    
    srand((unsigned int)time(NULL) + GetCurrentProcessId());
    
    for (int i = 0; i < 15; i++) {
        int random = rand() % 100;
        std::cout << "  Iteration " << (i+1) << ": Random value = " << random << std::endl;
        Sleep(1000);  // Wait 1 second
    }
    
    std::cout << "\nTest Process B completed!" << std::endl;
    std::cout << "===================================" << std::endl;
    
    return 0;
}
