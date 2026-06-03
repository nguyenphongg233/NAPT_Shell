#ifndef SIGNAL_HND_H
#define SIGNAL_HND_H

#include <windows.h>

// Global variable to store handle of running foreground process
extern HANDLE g_hForegroundProcess; 

void SetupSignalHandler();

#endif