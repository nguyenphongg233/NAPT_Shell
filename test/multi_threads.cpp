#include <windows.h>
#include <stdio.h>

int x = 0, y = 1;

void T1() {
    while (1) {
        x = y + 1;
        printf("%4d", x);
    }
}

void T2() {
    while (1) {
        y = 2;
        y *= 2;
    }
}

int main() {
    HANDLE h1, h2;
    DWORD ThreadID;
    h1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)T1, NULL, 0, &ThreadID);
    h2 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)T2, NULL, 0, &ThreadID);
    WaitForSingleObject(h1, INFINITE);
    WaitForSingleObject(h2, INFINITE);
    return 0;
}