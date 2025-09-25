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
    
    // Start the console thread
    if (!threadManager.StartConsoleThread()) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create console monitoring thread!");
        MessageBoxA(NULL, "Failed to create console monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Start the window thread
    if (!threadManager.StartWindowThread()) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create window thread!");
        MessageBoxA(NULL, "Failed to create window thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Both console and window threads started successfully!");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Press ESC in either thread to exit the application.");

    // Wait for threads to finish (ThreadManager handles all the complexity)
    threadManager.WaitForThreadsToFinish();

    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Application shutdown complete.");
    return 0;
}