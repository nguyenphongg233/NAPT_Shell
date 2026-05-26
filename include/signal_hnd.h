#ifndef SIGNAL_HND_H
#define SIGNAL_HND_H

#include <windows.h>

// Biến toàn cục lưu Handle của tiến trình Foreground đang chạy
extern HANDLE g_hForegroundProcess; 

void SetupSignalHandler();

#endif