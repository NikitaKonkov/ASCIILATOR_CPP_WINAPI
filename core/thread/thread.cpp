// Implementation of ThreadManager - handles console and window threads

#include "thread.hpp"
#include "../input/input.hpp"
#include "../window/window.hpp"
#include "../console/console.hpp"
#include "../clock/clock.hpp"
#include <cstdio>

////////////////////// Constructor and Destructor
ThreadManager::ThreadManager(volatile bool* globalExitFlag) : m_globalExitFlag(globalExitFlag) {
}

ThreadManager::~ThreadManager() {
    Cleanup();
}

////////////////////// Thread creation functions
bool ThreadManager::StartConsoleThread() {
    m_hConsoleThread = CreateThread(NULL, 0, ConsoleThreadProc, (LPVOID)m_globalExitFlag, 0, &m_consoleThreadId);
    return m_hConsoleThread != nullptr;
}

bool ThreadManager::StartWindowThread() {
    m_hWindowThread = CreateThread(NULL, 0, WindowThreadProc, (LPVOID)m_globalExitFlag, 0, &m_windowThreadId);
    return m_hWindowThread != nullptr;
}

////////////////////// Thread synchronization
void ThreadManager::WaitForThreadsToFinish() {
    if (!m_hConsoleThread || !m_hWindowThread) return;
    
    HANDLE threads[2] = { m_hConsoleThread, m_hWindowThread };
    
    // Wait for either thread to signal exit or finish
    while (!*m_globalExitFlag) {
        DWORD result = WaitForMultipleObjects(2, threads, FALSE, 100); // 100ms timeout
        
        if (result == WAIT_OBJECT_0) {
            // Console thread finished
            printf("Console thread finished.\n");
            break;
        } else if (result == WAIT_OBJECT_0 + 1) {
            // Window thread finished
            printf("Window thread finished.\n");
            break;
        } else if (result == WAIT_TIMEOUT) {
            // Check for global exit condition
            continue;
        } else {
            // Error occurred
            break;
        }
    }
    
    printf("ThreadManager initiating shutdown...\n");

    // Signal both threads to exit
    SignalExit();
    
    // Wait for both threads to finish with timeout
    if (WaitForMultipleObjects(2, threads, TRUE, 3000) == WAIT_TIMEOUT) {
        printf("Threads didn't exit cleanly, force terminating...\n");
        ForceTerminateThreads();
    }
}

void ThreadManager::SignalExit() {
    if (m_globalExitFlag) {
        *m_globalExitFlag = true;
    }
}

////////////////////// Cleanup functions
void ThreadManager::ForceTerminateThreads() {
    if (m_hConsoleThread) {
        TerminateThread(m_hConsoleThread, 0);
    }
    if (m_hWindowThread) {
        TerminateThread(m_hWindowThread, 0);
    }
}

void ThreadManager::Cleanup() {
    // Close thread handles
    if (m_hConsoleThread) {
        CloseHandle(m_hConsoleThread);
        m_hConsoleThread = nullptr;
    }
    if (m_hWindowThread) {
        CloseHandle(m_hWindowThread);
        m_hWindowThread = nullptr;
    }
}

////////////////////// Thread procedures (moved from main.cpp)
DWORD WINAPI ConsoleThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    // Create ConsoleManager, InputManager, ClockManager for console thread
    InputManager input;
    ClockManager clock;
    ConsoleManager console;

    int input_clock = clock.CreateClock(60, "InputClock"); // 60 FPS for input handling

    while (!*g_shouldExit) {
        input.SetMousePosition(400, 300); // Center mouse for testing
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
    
    // Create WindowManager for window thread
    WindowManager window;
    
    // Setup window with simple API call
    if (!window.SetupWindow(600, 400, "ASCIILATOR Text Window")) {
        MessageBoxA(NULL, "Failed to create text window!", "Error", MB_OK | MB_ICONERROR);
        *g_shouldExit = true;
        return 1;
    }
    
    // Run the window thread main loop with global exit flag
    window.RunWindowThread(g_shouldExit);
    
    return 0;
}