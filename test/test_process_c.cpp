#include <iostream>
#include <iomanip>
#include <cmath>
#include <windows.h>

int main() {
    std::cout << "========== Test Process C ==========" << std::endl;
    std::cout << "Process ID: " << GetCurrentProcessId() << std::endl;
    std::cout << "This process performs mathematical calculations.\n" << std::endl;
    
    std::cout << std::fixed << std::setprecision(4);
    
    std::cout << "Square root calculations:" << std::endl;
    for (double i = 1; i <= 10; i++) {
        std::cout << "  sqrt(" << i << ") = " << std::sqrt(i) << std::endl;
        Sleep(200);
    }
    
    std::cout << "\nPower calculations:" << std::endl;
    for (int i = 1; i <= 5; i++) {
        std::cout << "  2^" << i << " = " << std::pow(2, i) << std::endl;
        Sleep(200);
    }
    
    std::cout << "\nTest Process C completed!" << std::endl;
    std::cout << "===================================" << std::endl;
    
    return 0;
}
