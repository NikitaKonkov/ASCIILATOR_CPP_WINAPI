#include <windows.h>
#include "console/console.hpp"
#include "thread/thread.hpp"

// Shared exit flag
static volatile bool g_shouldExit = false;

int main() {
    // Create ConsoleManager for main thread (for initial setup and error reporting)
    ConsoleManager console;
    
    // Create ThreadManager to handle all threading
    ThreadManager threadManager(&g_shouldExit);
    
    // Start the console thread with ID
    if (!threadManager.CreateThread("console_input", ThreadType::CONSOLE_THREAD)) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create console monitoring thread!");
        MessageBoxA(NULL, "Failed to create console monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Start the window thread with ID
    if (!threadManager.CreateThread("window_gui", ThreadType::WINDOW_THREAD)) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create window thread!");
        MessageBoxA(NULL, "Failed to create window thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Start the sound thread with ID
    if (!threadManager.CreateThread("sound_audio", ThreadType::SOUND_THREAD)) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create sound thread!");
        MessageBoxA(NULL, "Failed to create sound thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Start the render thread with ID
    if (!threadManager.CreateThread("render_graphics", ThreadType::RENDER_THREAD)) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create render thread!");
        MessageBoxA(NULL, "Failed to create render thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "All threads started successfully!");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Press ESC to exit, P/S/T for sound, WASD + mouse for 3D movement");

    // Wait for threads to finish (ThreadManager handles all the complexity)
    threadManager.WaitForThreadsToFinish();

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Application shutdown complete.");
    return 0;
}