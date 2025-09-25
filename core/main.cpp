#include <windows.h>
#include "console/console.hpp"
#include "input/input.hpp"
#include "window/window.hpp"
#include "clock/clock.hpp"
#include "sound/sound.hpp"
#include "render/render.hpp"

// Shared exit flag
static volatile bool g_shouldExit = false;

// Thread procedures
DWORD WINAPI ConsoleThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    // Create ConsoleManager, InputManager, ClockManager for console thread
    InputManager input;
    ClockManager clock;
    ConsoleManager console;

    int input_clock = clock.CreateClock(60, "InputClock"); // 60 FPS for input handling
    
    while (!*g_shouldExit) {
        if (clock.SyncClock(input_clock)) {
            console.MoveCursor(1, 1);
        }
        if (input.GetKeyMSB(VK_ESCAPE)) {
            clock.DestroyAllClocks();
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Escape key pressed. Exiting console thread.");
            *g_shouldExit = true;
            break;
        }
        
        // Check exit flag more frequently
        Sleep(1);
    }
    return 0;
}

DWORD WINAPI WindowThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    // Create WindowManager and ClockManager for window thread
    WindowManager window;
    ClockManager clock;
    
    // Setup window with simple API call
    if (!window.SetupWindow(600, 400, "ASCIILATOR Text Window")) {
        MessageBoxA(NULL, "Failed to create text window!", "Error", MB_OK | MB_ICONERROR);
        *g_shouldExit = true;
        return 1;
    }
    
    // Setup clocks for this thread
    int windowClock = clock.CreateClock(5, "WindowUpdate"); // 5 FPS updates
    int heartbeatClock = clock.CreateClock(1, "WindowHeartbeat"); // 1 FPS heartbeat
    
    int exitAttempts = 0;
    while (!window.ShouldClose()) {
        // Check global exit flag if provided
        if (g_shouldExit && *g_shouldExit) {
            break;
        }
        
        // Process window messages
        window.ProcessWindowMessages();
        
        // Check for ESC key - highest priority exit condition
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            if (g_shouldExit) *g_shouldExit = true;
            window.SetShouldClose(true);
            exitAttempts++;
            if (exitAttempts > 10) { // Force exit after multiple ESC presses
                break;
            }
        }
        
        // Check window close state
        if (window.ShouldClose()) {
            if (g_shouldExit) *g_shouldExit = true;
            break;
        }
        
        // Update mouse delta at 5 FPS
        if (clock.SyncClock(windowClock)) {
            window.UpdateMouseDelta();
        }
        
        // Print heartbeat at 1 FPS
        if (clock.SyncClock(heartbeatClock)) {
            window.PrintHeartbeat();
        }
        
    }
    
    // Clean up resources
    clock.DestroyAllClocks();
    window.CloseWindow();
    
    return 0;
}

DWORD WINAPI SoundThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    InputManager input;
    SoundManager sound;
    ConsoleManager console;

    // Initialize audio system
    if (!sound.AudioInit()) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "Failed to initialize audio system!");
        *g_shouldExit = true;
        return 1;
    }

    sound.LoadWavFile("ahem_x.wav");
    sound.LoadWavFile("air_raid.wav");
    sound.LoadWavFile("airplane.wav");

    while (!*g_shouldExit) {
        if (input.GetKeyMSB(VK_ESCAPE)) {
            *g_shouldExit = true;
            break;
        }
        if (input.GetKeyMSB('1')) {
            sound.SoundWavRepeat(100,"ahem_x.wav", 0.5f);
        }
        if (input.GetKeyMSB('2')) {
            sound.SoundWavRepeat(101,"air_raid.wav", 0.5f);
        }
        if (input.GetKeyMSB('3')) {
            sound.SoundWavRepeat(102,"airplane.wav", 0.5f);
        }
        if (input.GetKeyMSB('4')) {
            sound.SoundWavKillAll();
        }   
        Sleep(10); // Polling interval
    }
    sound.SoundWavKillAll();
    sound.AudioShutdown();
    
    return 0;
}

DWORD WINAPI RenderThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    ConsoleManager console;
    ClockManager clock;
    SimpleRenderer renderer(&console);
    
    // Try to load the african head model
    std::string modelPath = "core/tinyrenderer-master/obj/african_head/african_head.obj";
    if (!renderer.LoadModel(modelPath)) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "Failed to load 3D model!");
        *g_shouldExit = true;
        return 1;
    }
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "3D renderer started! Model loaded successfully.");
    
    //int renderClock = clock.CreateClock(24, "RenderClock"); // 2 FPS for rendering
    
    while (!*g_shouldExit) {
        //if (clock.SyncClock(renderClock)) {
            console.MoveCursor(1, 1);
            renderer.RenderFrame();
        //}
        // Sleep(10); // Small sleep to prevent busy waiting
    }
    
    clock.DestroyAllClocks();
    return 0;
}

int main() {
    // Create ConsoleManager for main thread (for initial setup and error reporting)
    ConsoleManager console;
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Starting ASCIILATOR application...");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Press ESC to exit, 1/2/3 for sound, WASD + mouse for 3D movement");
    
    // Create thread handles
    HANDLE consoleThread = NULL;
    HANDLE windowThread = NULL;
    HANDLE soundThread = NULL;
    HANDLE renderThread = NULL;
    
    // Create console thread
    DWORD consoleThreadId;
    consoleThread = CreateThread(NULL, 0, ConsoleThreadProc, (LPVOID)&g_shouldExit, 0, &consoleThreadId);
    if (!consoleThread) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create console monitoring thread!");
        MessageBoxA(NULL, "Failed to create console monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // Create window thread
    DWORD windowThreadId;
    windowThread = CreateThread(NULL, 0, WindowThreadProc, (LPVOID)&g_shouldExit, 0, &windowThreadId);
    if (!windowThread) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create window thread!");
        MessageBoxA(NULL, "Failed to create window thread!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(consoleThread);
        return 2;
    }
    
    // Create sound thread
    DWORD soundThreadId;
    soundThread = CreateThread(NULL, 0, SoundThreadProc, (LPVOID)&g_shouldExit, 0, &soundThreadId);
    if (!soundThread) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create sound thread!");
        MessageBoxA(NULL, "Failed to create sound thread!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(consoleThread);
        CloseHandle(windowThread);
        return 3;
    }
    
    // Create render thread
    DWORD renderThreadId;
    renderThread = CreateThread(NULL, 0, RenderThreadProc, (LPVOID)&g_shouldExit, 0, &renderThreadId);
    if (!renderThread) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create render thread!");
        MessageBoxA(NULL, "Failed to create render thread!", "Error", MB_OK | MB_ICONERROR);
        CloseHandle(consoleThread);
        CloseHandle(windowThread);
        CloseHandle(soundThread);
        return 4;
    }
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "All threads started successfully!");
    
    // Create array of thread handles for waiting
    HANDLE threads[] = {consoleThread, windowThread, soundThread, renderThread};
    
    // Wait for any thread to finish or exit signal
    while (!g_shouldExit) {
        DWORD result = WaitForMultipleObjects(4, threads, FALSE, 100); // 100ms timeout
        
        if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + 4) {
            // One of the threads finished
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "A thread has finished, initiating shutdown...");
            break;
        } else if (result == WAIT_TIMEOUT) {
            // Continue checking
            continue;
        } else {
            // Error occurred
            break;
        }
    }
    
    // Signal all threads to exit
    g_shouldExit = true;
    
    // Wait for all threads to finish with timeout
    if (WaitForMultipleObjects(4, threads, TRUE, 3000) == WAIT_TIMEOUT) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "Threads didn't exit cleanly, force terminating...");
        TerminateThread(consoleThread, 0);
        TerminateThread(windowThread, 0);
        TerminateThread(soundThread, 0);
        TerminateThread(renderThread, 0);
    }
    
    // Clean up thread handles
    CloseHandle(consoleThread);
    CloseHandle(windowThread);
    CloseHandle(soundThread);
    CloseHandle(renderThread);
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Application shutdown complete.");
    return 0;
}