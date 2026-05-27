#include "../include/signal_hnd.h"
#include <iostream>

HANDLE g_hForegroundProcess = NULL;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType) {
    switch (fdwCtrlType) {
        case CTRL_C_EVENT:
            // If there's a foreground process running, terminate it
            if (g_hForegroundProcess != NULL) {
                TerminateProcess(g_hForegroundProcess, 0);
                std::cout << "\n[Foreground process terminated by CTRL+C]\n";
            } else {
                std::cout << "\nTinyShell> "; // Return to input prompt if shell is idle
            }
            return TRUE; // Prevent Windows from terminating TinyShell
        default:
            return FALSE;
    }
}

void SetupSignalHandler() {
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
}