#include <windows.h>
#include "input/input.hpp"
#include "display/display.hpp"
#include "clock/clock.hpp"
#include "sound/sound.hpp"

static float angle = 0.0f;


// Thread function
DWORD WINAPI ConsoleThread(LPVOID lpParam) {
    // Create DisplayManager, InputManager, ClockManager, and SoundManager instances
    DisplayManager display;
    InputManager input;
    ClockManager clockManager;
    SoundManager soundManager;
    
    // Initialize sound system
    if (!soundManager.AudioInit()) {
        display.PrintColoredLine(COLOR_BRIGHT_RED, "ERROR: Failed to initialize audio system!");
        return 1;
    }
    display.PrintColoredLine(COLOR_BRIGHT_GREEN, "Audio system initialized successfully!");
    
    // Load test WAV files
    soundManager.LoadWavFile("ahem_x.wav");
    soundManager.LoadWavFile("air_raid.wav");
    soundManager.LoadWavFile("airplane_chime_x.wav");
    
    // Create different clocks for different purposes
    int mainLoopClock = clockManager.CreateClock(60, "MainLoop");     // 60 FPS main loop
    int displayClock = clockManager.CreateClock(1, "Display");       // 1 FPS display updates
    
    // Performance counters
    int frameCount = 0;
    bool showDetailedInfo = false;
    bool soundTestMode = false; // Toggle between FPS adjustment and sound testing

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
                display.PrintColoredLine(COLOR_CYAN, "Test: Keys, Mouse buttons (L/R/M), Mouse movement | Press 'I' for clock info | 'S' for sound mode");
                
                // Display current mode
                if (soundTestMode) {
                    display.PrintColoredLine(COLOR_BRIGHT_MAGENTA, "SOUND TEST MODE: 1-6=Tones (spatial), 7-9=WAV files, 0=Spinning sound | 'S' to switch back");
                } else {
                    display.PrintColoredLine(COLOR_BRIGHT_CYAN, "FPS ADJUST MODE: 1=30fps, 2=60fps, 3=120fps | 'S' for sound testing");
                }
                
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
                    clockManager.GetDeltaTime(mainLoopClock));
                
                // Sound System Status
                display.MoveCursor(7, 1);
                display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_MAGENTA, "Sound System Status:");
                display.MoveCursor(8, 1);
                if (soundTestMode) {
                    display.PrintColoredLine(COLOR_BRIGHT_GREEN, "READY - Press 1-6 for tones, 7-9 for WAV files, 0 for spinning demo");
                } else {
                    display.PrintColoredLine(COLOR_BRIGHT_YELLOW, "FPS Mode - Press 'S' to enable sound testing");
                }
                
                // Separator
                display.MoveCursor(9, 1);
                display.DrawHorizontalLine(1, 9, 80, '-');
                
                // Test keyboard input
                display.MoveCursor(11, 1);
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
                }
            }
        }
        display.MoveCursor(18, 1);
        display.PrintFormatted("Sound angle: %f", angle);

        // Toggle detailed info with 'I' key
        if (input.GetKeyLSB(VK_I)) {
            showDetailedInfo = !showDetailedInfo;
        }
        
        // Toggle sound test mode with 'S' key
        if (input.GetKeyLSB(VK_S)) {
            soundTestMode = !soundTestMode;
            soundManager.SoundKillAll(); // Stop all sounds when switching modes
            soundManager.SoundWavKillAll();
        }
        
        // Sound testing or FPS adjustment based on mode
        if (soundTestMode) {


            // Sound testing mode - Number keys 1-9 and 0 for different sounds
            if (input.GetKeyLSB(VK_1) && !soundManager.SoundIsPlaying(1)) {
                soundManager.SoundKillAll(); // Stop previous sounds
                soundManager.SoundTimer(1, 261.63, 0.8f, 0.0, 5.0); // C4 note
                soundManager.SoundTimer(2, 293.66, 0.8f, 0.0, 7.0); // D4 note
                soundManager.SoundTimer(3, 329.63, 0.8f, 0.0, 9.0); // E4 note
            }
            if (input.GetKeyLSB(VK_2) && !soundManager.SoundIsPlaying(4)) {
                soundManager.SoundKillAll();
                soundManager.SoundStarterStatic(4, 440.0, 1.0f, 0.0, 0.0); // Center
            }

            if (input.GetKeyLSB(VK_3)) {
                soundManager.SoundKillAll();
            }

            if (input.GetKeyLSB(VK_8)) {
                angle -= 1.0f;
                soundManager.SoundAngle(100 + 0, angle); // Center
            }
            if (input.GetKeyLSB(VK_9)) {
                angle += 1.0f;
                soundManager.SoundAngle(100 + 0, angle); // Center
            }
            if (input.GetKeyLSB(VK_0)) {
                soundManager.SoundWavKillAll();
                soundManager.SoundWavTimer(0, "air_raid.wav", 1.0f, 100.0);
                soundManager.SoundAngle(100 + 0, angle); // Center
            }


        } else {
            // FPS adjustment mode (original functionality)
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
        }
        
        // Check for exit condition
        if (input.GetKeyMSB(VK_ESCAPE)) {
            // Show final statistics
            display.MoveCursor(25, 1);
            display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_RED, "ESC pressed - Showing final statistics...");
            
            display.MoveCursor(27, 1);
            display.PrintStyledText(STYLE_BOLD, COLOR_BRIGHT_WHITE, "Final Clock Statistics:");
            clockManager.ListAllClocks();
            
            break;
        }
    }
    
    // Cleanup audio system
    soundManager.SoundKillAll();
    soundManager.SoundWavKillAll();
    soundManager.AudioShutdown();
    display.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Audio system shutdown complete.");
    
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
    

    return 0;
}