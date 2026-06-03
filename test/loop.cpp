#include <iostream>
#include <windows.h>
int main() {
    // in ra thowfi gian chajyua cua tien trinh con

    double cpuTime = 0.0;
    FILETIME creationTime, exitTime, kernelTime, userTime;
    
    int id = 0;
    while (true) {  
        std::cout << "[Child Process] Dang chay tu dong...\n";
        Sleep(3000); // Nghỉ 3 giây rồi in tiếp
        id++;
        std :: cout << "ID: " << id << "\n";
    }
    return 0;
}