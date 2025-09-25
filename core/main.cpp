#include <windows.h>
#include "input/input.hpp"
#include "window/window.hpp"
#include "console/console.hpp"
#include "clock/clock.hpp"
#include "sound/sound.hpp"
#include "render/render.hpp"

// Shared exit flag
static volatile bool g_shouldExit = false;

// Console thread function
DWORD WINAPI ConsoleThread(LPVOID lpParam) {
    // Create ConsoleManager, InputManager, ClockManager for console thread
    InputManager input;
    ClockManager clock;
    ConsoleManager console;

    int input_clock = clock.CreateClock(60, "InputClock"); // 60 FPS for input handling

    while (!g_shouldExit) {
        input.SetMousePosition(400, 300); // Center mouse for testing
        if (clock.SyncClock(input_clock)) {
            console.MoveCursor(1, 1);
        }
        if (input.GetKeyMSB(VK_ESCAPE)) {
            clock.DestroyAllClocks();
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Escape key pressed. Exiting console thread.");
            g_shouldExit = true;
            break;
        }
        
        // Check exit flag more frequently
        Sleep(1);
    }

    return 0;
}

// Window thread function
DWORD WINAPI WindowThread(LPVOID lpParam) {
    // Create WindowManager and ClockManager for window thread
    WindowManager window;
    ClockManager clock;
    
    // Initialize and show a small text window
    window.SetWindowSize(600, 400);
    if (!window.InitiatelizeWindow() || !window.CreateTextWindow()) {
        MessageBoxA(NULL, "Failed to create text window!", "Error", MB_OK | MB_ICONERROR);
        g_shouldExit = true;
        return 1;
    }
    window.SetWindowTitle("ASCIILATOR Text Window");
    window.InitializeRawInput();

    // Clock for updating the window text with mouse delta
    int windowClock = clock.CreateClock(5, "WindowUpdate"); // 5 FPS updates to reduce CPU usage
    int mainThreadClock = clock.CreateClock(1, "WindowHeartbeat"); // 1 FPS for heartbeat

    // Window thread main loop
    MSG msg{};
    int exitAttempts = 0;
    while (!g_shouldExit && !window.ShouldClose()) {
        // Pump window messages so raw input and UI work
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_shouldExit = true;
                goto window_exit;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check for ESC key first - highest priority exit condition
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            g_shouldExit = true;
            window.SetShouldClose(true);
            exitAttempts++;
            if (exitAttempts > 10) { // If ESC pressed multiple times, force exit
                break;
            }
        }

        // Check window close state
        if (window.ShouldClose()) {
            g_shouldExit = true;
            break;
        }

        if (clock.SyncClock(windowClock)) {
            unsigned long dx = 0, dy = 0;
            window.GetDeltaPosition(&dx, &dy);
            long sdx = (long)dx;
            long sdy = (long)dy;
            if (sdx != 0 || sdy != 0) {
                // Clear the window instead of moving cursor since cursor movement doesn't work
                window.ClearWindow();
                window.PrintToWindow("Mouse delta: %ld, %ld\r\n", sdx, sdy);
            }
        }

        if (clock.SyncClock(mainThreadClock)) {
            // Once per second, clear and show heartbeat to prevent text accumulation
            window.ClearWindow();
            window.PrintToWindow("[Window] heartbeat...\r\n");
        }

        // Sleep(10); // Shorter sleep for better responsiveness
    }

window_exit:
    // Clean up window resources
    clock.DestroyAllClocks();
    window.CloseWindow();
    return 0;
}

int main() {
    HANDLE hConsoleThread, hWindowThread;
    DWORD consoleThreadId, windowThreadId;

    // Create ConsoleManager for main thread (for initial setup and error reporting)
    ConsoleManager console;
    
    // Create the console thread
    hConsoleThread = CreateThread(NULL, 0, ConsoleThread, NULL, 0, &consoleThreadId);
    if (hConsoleThread == NULL) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create console monitoring thread!");
        MessageBoxA(NULL, "Failed to create console monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Create the window thread
    hWindowThread = CreateThread(NULL, 0, WindowThread, NULL, 0, &windowThreadId);
    if (hWindowThread == NULL) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create window thread!");
        MessageBoxA(NULL, "Failed to create window thread!", "Error", MB_OK | MB_ICONERROR);
        
        // Clean up console thread
        g_shouldExit = true;
        WaitForSingleObject(hConsoleThread, 2000);
        CloseHandle(hConsoleThread);
        return 1;
    }

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Both console and window threads started successfully!");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Press ESC in either thread to exit the application.");

    // Main thread now just waits for both threads to finish
    HANDLE threads[2] = { hConsoleThread, hWindowThread };
    
    // Wait for either thread to signal exit or finish
    while (!g_shouldExit) {
        DWORD result = WaitForMultipleObjects(2, threads, FALSE, 100); // 100ms timeout
        
        if (result == WAIT_OBJECT_0) {
            // Console thread finished
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Console thread finished.");
            break;
        } else if (result == WAIT_OBJECT_0 + 1) {
            // Window thread finished
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Window thread finished.");
            break;
        } else if (result == WAIT_TIMEOUT) {
            // Check for global exit condition
            continue;
        } else {
            // Error occurred
            break;
        }
    }

    console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Main thread initiating shutdown...");

    // Signal both threads to exit
    g_shouldExit = true;
    
    // Wait for both threads to finish with timeout
    if (WaitForMultipleObjects(2, threads, TRUE, 3000) == WAIT_TIMEOUT) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "Threads didn't exit cleanly, force terminating...");
        // Force terminate if threads don't exit cleanly
        TerminateThread(hConsoleThread, 0);
        TerminateThread(hWindowThread, 0);
    }
    
    // Close thread handles
    CloseHandle(hConsoleThread);
    CloseHandle(hWindowThread);

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Application shutdown complete.");
    return 0;
}