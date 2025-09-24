#include <windows.h>
#include "input/input.hpp"
#include "display/display.hpp"
#include "clock/clock.hpp"

// Thread function
DWORD WINAPI ConsoleThread(LPVOID lpParam) {
    // Create DisplayManager, InputManager, and ClockManager instances
    DisplayManager display;
    InputManager input;
    ClockManager clockManager;
    
    // Create different clocks for different purposes
    int mainLoopClock = clockManager.CreateClock(60, "MainLoop");     // 60 FPS main loop
    int displayClock = clockManager.CreateClock(30, "Display");       // 30 FPS display updates
    int inputClock = clockManager.CreateClock(120, "Input");          // 120 FPS input polling
    
    // Performance counters
    int frameCount = 0;
    bool showDetailedInfo = false;

    while (true) {
        // Main loop timing
        if (clockManager.SyncClock(mainLoopClock)) {
            frameCount++;
            
            // Display updates (30 FPS)
            if (clockManager.SyncClock(displayClock)) {
                // Clear screen and display header
                display.ClearScreen();
                display.MoveCursor(1, 1);
                display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_GREEN, "ASCIILATOR System Test - Press ESC to exit");
                display.PrintColoredLine(COLOR_CYAN, "Test: Keys, Mouse buttons (L/R/M), Mouse movement | Press 'I' for detailed clock info");
                
                // Clock status display
                display.MoveCursor(3, 1);
                display.DrawHorizontalLine(1, 3, 80, '=');
                display.MoveCursor(4, 1);
                display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_YELLOW, "Clock Status:");
                
                display.MoveCursor(5, 1);
                display.PrintFormatted("Main Loop: %.1f FPS (Target: %d) | Frames: %lu | Uptime: %.1fs",
                    clockManager.GetCurrentFps(mainLoopClock),
                    clockManager.GetTargetFps(mainLoopClock),
                    clockManager.GetTotalFrames(mainLoopClock),
                    clockManager.GetUptime(mainLoopClock));
                
                display.MoveCursor(6, 1);
                display.PrintFormatted("Display: %.1f FPS | Input: %.1f FPS | Delta: %.3fs",
                    clockManager.GetCurrentFps(displayClock),
                    clockManager.GetCurrentFps(inputClock),
                    clockManager.GetDeltaTime(mainLoopClock));
                
                // Separator
                display.MoveCursor(7, 1);
                display.DrawHorizontalLine(1, 7, 80, '-');
                
                // Test keyboard input
                display.MoveCursor(9, 1);
                display.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Keyboard Status:");
                input.PrintPressedKeys();
                
                // Test specific key combinations
                if (input.GetKeyMSB(VK_W)) {
                    display.PrintColoredLine(COLOR_BRIGHT_GREEN, "W key is pressed!");
                }
                
                if (input.GetPressedKeys(2, VK_CONTROL, VK_A)) {
                    display.PrintColoredLine(COLOR_BRIGHT_RED, "Ctrl+A combination detected!");
                }
                
                // Test mouse input
                display.PrintColoredLine(COLOR_BRIGHT_CYAN, "Mouse Status:");
                input.PrintMousePosition();
                input.PrintMouseButtons();
                
                // Check for mouse movement
                if (input.IsMouseMoved()) {
                    display.PrintColoredLine(COLOR_BRIGHT_MAGENTA, "Mouse moved!");
                }
                
                // Test specific mouse button combinations
                if (input.GetMouseButtonState(VK_LBUTTON) && input.GetMouseButtonState(VK_RBUTTON)) {
                    display.PrintColoredLine(COLOR_BRIGHT_RED, "Both left and right buttons pressed!");
                }
                
                // Show detailed clock information if requested
                if (showDetailedInfo) {
                    display.MoveCursor(18, 1);
                    display.DrawHorizontalLine(1, 18, 80, '=');
                    display.MoveCursor(19, 1);
                    display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_CYAN, "Detailed Clock Information:");
                    
                    display.MoveCursor(20, 1);
                    clockManager.PrintClockInfo(mainLoopClock);
                    display.MoveCursor(27, 1);
                    clockManager.PrintClockInfo(displayClock);
                    display.MoveCursor(34, 1);
                    clockManager.PrintClockInfo(inputClock);
                }
            }
        }
        
        // High-frequency input polling (120 FPS)
        if (clockManager.SyncClock(inputClock)) {
            // Toggle detailed info with 'I' key
            if (input.GetKeyLSB(VK_I)) {
                showDetailedInfo = !showDetailedInfo;
            }
            
            // Dynamic FPS adjustment with number keys
            if (input.GetKeyLSB(VK_1)) {
                clockManager.SetClockFps(mainLoopClock, 30);
                clockManager.ResetCounters(mainLoopClock);
            }
            if (input.GetKeyLSB(VK_2)) {
                clockManager.SetClockFps(mainLoopClock, 60);
                clockManager.ResetCounters(mainLoopClock);
            }
            if (input.GetKeyLSB(VK_3)) {
                clockManager.SetClockFps(mainLoopClock, 120);
                clockManager.ResetCounters(mainLoopClock);
            }
            
            // Check for exit condition
            if (input.GetKeyMSB(VK_ESCAPE)) {
                // Show final statistics
                display.MoveCursor(25, 1);
                display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_RED, "ESC pressed - Showing final statistics...");
                
                display.MoveCursor(27, 1);
                display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_WHITE, "Final Clock Statistics:");
                clockManager.ListAllClocks();
                
                Sleep(3000); // Show stats for 3 seconds
                break;
            }
        }
        
        // Small sleep to prevent excessive CPU usage
        Sleep(1);
    }
    
    return 0;
}

int main() {
    HANDLE hThread;
    DWORD threadId;

    // Create DisplayManager and ClockManager for main thread
    DisplayManager display;
    ClockManager clockManager;
    
    // Create a clock for main thread operations
    int mainThreadClock = clockManager.CreateClock(1, "MainThread"); // 1 FPS for main thread updates
    
    display.SetTitle("ASCIILATOR System Test - Input, Display & Clock Integration");

    // Clear console and show initial message
    display.ClearScreen();
    display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_BLUE, "ASCIILATOR System Integration Test");
    display.PrintLine("");
    display.PrintColoredLine(COLOR_BRIGHT_WHITE, "Features demonstrated:");
    display.PrintColored(COLOR_BRIGHT_GREEN, "  • Input System: ");
    display.PrintLine("Keyboard and mouse detection");
    display.PrintColored(COLOR_BRIGHT_CYAN, "  • Display System: ");
    display.PrintLine("ANSI colors, cursor control, text formatting");
    display.PrintColored(COLOR_BRIGHT_YELLOW, "  • Clock System: ");
    display.PrintLine("Multi-clock FPS management and timing");
    display.PrintLine("");
    
    display.PrintColoredLine(COLOR_BRIGHT_WHITE, "Interactive Controls:");
    display.PrintLine("  • ESC: Exit application");
    display.PrintLine("  • I: Toggle detailed clock information");
    display.PrintLine("  • 1/2/3: Set main loop to 30/60/120 FPS");
    display.PrintLine("  • W/WASD: Test key detection");
    display.PrintLine("  • Ctrl+A: Test key combinations");
    display.PrintLine("  • Mouse: Move and click to test mouse input");
    display.PrintLine("");
    
    display.PrintColoredLine(COLOR_BRIGHT_MAGENTA, "Starting system monitoring thread...");
    display.PrintLine("Note: The test will run in this console. All systems are integrated.");

    // Create the thread
    hThread = CreateThread(NULL, 0, ConsoleThread, NULL, 0, &threadId);

    if (hThread == NULL) {
        display.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to create monitoring thread!");
        MessageBoxA(NULL, "Failed to create system monitoring thread!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // Main thread monitoring loop
    while (WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT) {
        if (clockManager.SyncClock(mainThreadClock)) {
            // Update main thread clock info (every second)
            // Could add main thread specific monitoring here
        }
        Sleep(100); // Check thread status every 100ms
    }

    // Close the thread handle
    CloseHandle(hThread);

    // Final message with timing
    display.ClearScreen();
    display.MoveCursor(10, 20);
    display.DrawBox(15, 8, 50, 8, COLOR_BRIGHT_GREEN);
    display.MoveCursor(10, 20);
    display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_GREEN, "System Integration Test Complete!");
    display.MoveCursor(11, 20);
    display.PrintFormatted("Main thread uptime: %.2f seconds", clockManager.GetUptime(mainThreadClock));
    display.MoveCursor(12, 20);
    display.PrintColoredLine(COLOR_BRIGHT_CYAN, "All systems performed successfully!");
    display.MoveCursor(13, 20);
    display.PrintLine("Thank you for testing ASCIILATOR!");
    
    // Wait a moment before exiting
    Sleep(3000);

    return 0;
}