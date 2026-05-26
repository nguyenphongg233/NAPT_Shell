#include "../include/signal_hnd.h"
#include <iostream>

HANDLE g_hForegroundProcess = NULL;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
            // Nếu có tiến trình Foreground đang chạy, ép buộc tắt nó
            if (g_hForegroundProcess != NULL) {
                TerminateProcess(g_hForegroundProcess, 0);
                std::cout << "\n[Foreground process terminated by CTRL+C]\n";
            } else {
                std::cout << "\nTinyShell> "; // Trả lại giao diện nhập nếu shell đang trống
            }
            return TRUE; // Ngăn không cho Windows tắt TinyShell
        default:
            return FALSE;
    }
}

void SetupSignalHandler() {
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
}