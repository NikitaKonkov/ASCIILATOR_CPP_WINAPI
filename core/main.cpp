#include <windows.h>
#include "input/input.hpp"
#include "display/display.hpp"
#include "clock/clock.hpp"
#include "sound/sound.hpp"
#include "render/render.hpp"

// Thread function
DWORD WINAPI ConsoleThread(LPVOID lpParam) {
    // Create DisplayManager, InputManager, ClockManager, SoundManager, and RenderManager instances
    DisplayManager display;
    InputManager input;
    ClockManager clock;
    SoundManager sound;
    RenderManager render;
    
    return 0;
}

int main() {
    HANDLE hThread;
    DWORD threadId;

    // Create DisplayManager and ClockManager for main thread
    DisplayManager display;
    ClockManager clock;
    
    // Create a clock for main thread operations
    int mainThreadClock = clock.CreateClock(1, "MainThread"); // 1 FPS for main thread updates

    // Create the thread
    hThread = CreateThread(NULL, 0, ConsoleThread, NULL, 0, &threadId);

    if (hThread == NULL) {
        display.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create monitoring thread!");
        MessageBoxA(NULL, "Failed to create system monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Main thread monitoring loop
    while (WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT) {
        if (clock.SyncClock(mainThreadClock)) {
            // Update main thread clock info (every second)
            // Could add main thread specific monitoring here
        }
        Sleep(100); // Check thread status every 100ms
    }

    // Close the thread handle
    CloseHandle(hThread);


    return 0;
}