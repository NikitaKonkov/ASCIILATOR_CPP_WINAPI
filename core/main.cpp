#include <windows.h>
#include "input/input.hpp"
#include "console/console.hpp"
#include "clock/clock.hpp"
#include "sound/sound.hpp"
#include "render/render.hpp"

// Thread function
DWORD WINAPI ConsoleThread(LPVOID lpParam) {

    // Create ConsoleManager, InputManager, ClockManager, SoundManager, and RenderManager instances
    InputManager input;
    ClockManager clock;
    ConsoleManager console;
    input.InitializeRawInput();

    int input_clock = clock.CreateClock(60, "InputClock"); // 60 FPS for input handling

    MSG msg;
    while (true) {
        if (clock.SyncClock(input_clock)) {
            console.MoveCursor(1, 1);
            
            // Process all pending messages
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        if (input.GetKeyMSB(VK_ESCAPE)) {
            console.PrintColoredLine(COLOR_RED, "Escape key pressed. Exiting thread.");
            break;
        }
        Sleep(1); // Sleep briefly to prevent CPU hogging
    }

    return 0;
}

int main() {
    HANDLE hThread;
    DWORD threadId;

    // Create ConsoleManager and ClockManager for main thread
    ConsoleManager console;
    ClockManager clock;
    
    // Create a clock for main thread operations
    int mainThreadClock = clock.CreateClock(1, "MainThread"); // 1 FPS for main thread updates

    // Create the thread
    hThread = CreateThread(NULL, 0, ConsoleThread, NULL, 0, &threadId);

    if (hThread == NULL) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create monitoring thread!");
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