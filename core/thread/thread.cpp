// Implementation of ThreadManager - handles console and window threads

#include "thread.hpp"
#include "../input/input.hpp"
#include "../window/window.hpp"
#include "../console/console.hpp"
#include "../clock/clock.hpp"
#include "../sound/sound.hpp"
#include "../render/render.hpp"
#include <cstdio>
#include <iostream>
#include <vector>

////////////////////// Constructor and Destructor
ThreadManager::ThreadManager(volatile bool* globalExitFlag) : m_globalExitFlag(globalExitFlag) {
}

ThreadManager::~ThreadManager() {
    Cleanup();
}

////////////////////// Generic thread creation
bool ThreadManager::CreateThread(const std::string& threadId, ThreadType type) {
    // Check if thread already exists
    if (m_threads.find(threadId) != m_threads.end()) {
        std::cout << "Thread '" << threadId << "' already exists!" << std::endl;
        return false;
    }
    
    ThreadInfo threadInfo;
    threadInfo.type = type;
    threadInfo.name = threadId;
    
    // Get the appropriate thread procedure
    LPTHREAD_START_ROUTINE threadProc = GetThreadProcedure(type);
    if (!threadProc) {
        std::cout << "Unknown thread type for '" << threadId << "'" << std::endl;
        return false;
    }
    
    // Create the thread
    threadInfo.handle = ::CreateThread(NULL, 0, threadProc, (LPVOID)m_globalExitFlag, 0, &threadInfo.threadId);
    
    if (!threadInfo.handle) {
        std::cout << "Failed to create thread '" << threadId << "'" << std::endl;
        return false;
    }
    
    // Store thread info
    m_threads[threadId] = threadInfo;
    std::cout << "Thread '" << threadId << "' created successfully (ID: " << threadInfo.threadId << ")" << std::endl;
    
    return true;
}

LPTHREAD_START_ROUTINE ThreadManager::GetThreadProcedure(ThreadType type) {
    switch (type) {
        case ThreadType::CONSOLE_THREAD:
            return ConsoleThreadProc;
        case ThreadType::WINDOW_THREAD:
            return WindowThreadProc;
        case ThreadType::SOUND_THREAD:
            return SoundThreadProc;
        case ThreadType::RENDER_THREAD:
            return RenderThreadProc;
        default:
            return nullptr;
    }
}

bool ThreadManager::IsThreadRunning(const std::string& threadId) {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end()) {
        return false;
    }
    
    DWORD exitCode;
    if (GetExitCodeThread(it->second.handle, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    
    return false;
}

////////////////////// Thread synchronization
void ThreadManager::WaitForThreadsToFinish() {
    if (m_threads.empty()) return;
    
    // Create array of handles
    std::vector<HANDLE> handles;
    for (auto& pair : m_threads) {
        if (pair.second.handle) {
            handles.push_back(pair.second.handle);
        }
    }
    
    if (handles.empty()) return;
    
    // Wait for any thread to signal exit or finish
    while (!*m_globalExitFlag) {
        DWORD result = WaitForMultipleObjects(handles.size(), handles.data(), FALSE, 100); // 100ms timeout
        
        if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + handles.size()) {
            // One of the threads finished
            HANDLE finishedHandle = handles[result - WAIT_OBJECT_0];
            
            // Find which thread finished
            for (auto& pair : m_threads) {
                if (pair.second.handle == finishedHandle) {
                    std::cout << "Thread '" << pair.first << "' finished." << std::endl;
                    break;
                }
            }
            break;
        } else if (result == WAIT_TIMEOUT) {
            // Check for global exit condition
            continue;
        } else {
            // Error occurred
            break;
        }
    }
    
    std::cout << "ThreadManager initiating shutdown..." << std::endl;

    // Signal all threads to exit
    SignalExit();
    
    // Wait for all threads to finish with timeout
    if (WaitForMultipleObjects(handles.size(), handles.data(), TRUE, 3000) == WAIT_TIMEOUT) {
        std::cout << "Threads didn't exit cleanly, force terminating..." << std::endl;
        KillAllThreads();
    }
}

void ThreadManager::WaitForThread(const std::string& threadId) {
    auto it = m_threads.find(threadId);
    if (it == m_threads.end() || !it->second.handle) {
        return;
    }
    
    WaitForSingleObject(it->second.handle, INFINITE);
    std::cout << "Thread '" << threadId << "' finished." << std::endl;
}

void ThreadManager::SignalExit() {
    if (m_globalExitFlag) {
        *m_globalExitFlag = true;
    }
}

////////////////////// Cleanup functions
void ThreadManager::KillThread(const std::string& threadId) {
    auto it = m_threads.find(threadId);
    if (it != m_threads.end() && it->second.handle) {
        TerminateThread(it->second.handle, 0);
        CloseHandle(it->second.handle);
        std::cout << "Thread '" << threadId << "' force terminated." << std::endl;
        m_threads.erase(it);
    }
}

void ThreadManager::KillAllThreads() {
    for (auto& pair : m_threads) {
        if (pair.second.handle) {
            TerminateThread(pair.second.handle, 0);
            std::cout << "Thread '" << pair.first << "' force terminated." << std::endl;
        }
    }
    Cleanup();
}

void ThreadManager::ForceTerminateThread(const std::string& threadId) {
    auto it = m_threads.find(threadId);
    if (it != m_threads.end() && it->second.handle) {
        TerminateThread(it->second.handle, 0);
    }
}

void ThreadManager::Cleanup() {
    // Close all thread handles
    for (auto& pair : m_threads) {
        if (pair.second.handle) {
            CloseHandle(pair.second.handle);
        }
    }
    m_threads.clear();
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

DWORD WINAPI SoundThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    // Create InputManager and SoundManager for sound thread
    InputManager input;
    SoundManager sound;
    ConsoleManager console;
    
    // Initialize audio system
    if (!sound.AudioInit()) {
        console.PrintColoredLine(COLOR_BRIGHT_RED, "Failed to initialize audio system!");
        *g_shouldExit = true;
        return 1;
    }
    
    // Load some test WAV files (optional, if available)
    sound.LoadWavFile("ahem_x.wav");
    sound.LoadWavFile("air_raid.wav");
    sound.LoadWavFile("airplane_chime_x.wav");
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Sound thread started!");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Controls: P = Play sound, S = Stop all sounds, T = Play tone");
    
    bool p_key_pressed = false;
    bool s_key_pressed = false;
    bool t_key_pressed = false;
    int current_sound_id = -1;
    int current_tone_id = -1;
    
    while (!*g_shouldExit) {
        // Check for P key press (Play WAV sound)
        bool p_current = input.GetKeyMSB('1');
        if (p_current && !p_key_pressed) {
            // Play a WAV file (cycle through available ones)
            static int wav_index = 0;
            const char* wav_files[] = {"ahem_x.wav", "air_raid.wav", "airplane_chime_x.wav"};
            const int num_wavs = sizeof(wav_files) / sizeof(wav_files[0]);
            
            current_sound_id = sound.SoundWavRepeat(10 + wav_index, wav_files[wav_index], 0.8f);
            console.PrintFormatted("Playing WAV: %s (ID: %d)\n", wav_files[wav_index], current_sound_id);
            
            wav_index = (wav_index + 1) % num_wavs;
        }
        p_key_pressed = p_current;
        
        // Check for S key press (Stop all sounds)
        bool s_current = input.GetKeyMSB('3');
        if (s_current && !s_key_pressed) {
            sound.SoundWavKillAll();
            sound.SoundKillAll();
            console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "All sounds stopped!");
            current_sound_id = -1;
            current_tone_id = -1;
        }
        s_key_pressed = s_current;
        
        // Check for T key press (Play tone)
        bool t_current = input.GetKeyMSB('2');
        if (t_current && !t_key_pressed) {
            // Play a simple tone (440 Hz A note)
            current_tone_id = sound.SoundTimer(5, 440.0, 0.5, 0.0, 2.0); // 2 second duration
            console.PrintFormatted("Playing tone: 440Hz (ID: %d)\n", current_tone_id);
        }
        t_key_pressed = t_current;
        
        // Small delay to prevent excessive CPU usage
        Sleep(16); // ~60 FPS
    }
    
    // Cleanup sound system
    sound.SoundWavKillAll();
    sound.SoundKillAll();
    sound.AudioShutdown();
    console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Sound thread finished.");
    
    return 0;
}

DWORD WINAPI RenderThreadProc(LPVOID lpParam) {
    volatile bool* g_shouldExit = static_cast<volatile bool*>(lpParam);
    
    // Create InputManager, RenderManager, and ClockManager for render thread
    InputManager input;
    RenderManager render;
    ClockManager clock;
    ConsoleManager console;
    
    // Initialize render system
    render.Initialize();
    
    console.PrintColoredLine(COLOR_BRIGHT_GREEN, "Render thread started!");
    console.PrintColoredLine(COLOR_BRIGHT_CYAN, "Controls: WASD = Move, Mouse = Look around, Space/Shift = Up/Down");
    
    // Create a clock for 60 FPS rendering
    int render_clock = clock.CreateClock(60, "RenderClock");
    
    // Create a clock for camera info display (update every 5 frames = 12 times per second)
    int info_clock = clock.CreateClock(12, "InfoClock");
    
    while (!*g_shouldExit) {
        // Update camera from mouse movement
        render.UpdateCameraFromMouse();
        
        // Handle keyboard movement input
        bool w_current = input.GetKeyMSB('W');
        bool s_current = input.GetKeyMSB('S');
        bool a_current = input.GetKeyMSB('A');
        bool d_current = input.GetKeyMSB('D');
        bool space_current = input.GetKeyMSB(VK_SPACE);
        bool shift_current = input.GetKeyMSB(VK_LSHIFT) || input.GetKeyMSB(VK_RSHIFT);
        
        // Move camera based on input
        render.MoveCameraKeyboard(
            w_current,      // forward
            s_current,      // backward
            a_current,      // left
            d_current,      // right
            space_current,  // up
            shift_current   // down
        );
        
        // Display camera info periodically
        if (clock.SyncClock(info_clock)) {
            float x, y, z, yaw, pitch;
            render.GetCameraPosition(&x, &y, &z);
            render.GetCameraRotation(&yaw, &pitch);
            
            // Convert radians to degrees for display
            float yaw_deg = yaw * (180.0f / 3.14159f);
            float pitch_deg = pitch * (180.0f / 3.14159f);
            
            // Display camera info in top-left corner (we don't have roll in this system)
            console.MoveCursor(1, 1);
            console.PrintFormatted("Pos: X=%.1f Y=%.1f Z=%.1f | Yaw=%.1f° Pitch=%.1f° | Use WASD+Mouse", 
                                 x, y, z, yaw_deg, pitch_deg);
        }
        
        // Render at 60 FPS
        if (clock.SyncClock(render_clock)) {
            // Begin frame
            render.BeginFrame();
            
            // Draw test objects (3 colored cubes)
            render.DrawTestObjects();
            
            // End frame and present
            render.EndFrame();
        }
        
        // Small delay to prevent excessive CPU usage
        Sleep(1);
    }
    
    // Cleanup
    clock.DestroyAllClocks();
    console.PrintColoredLine(COLOR_BRIGHT_YELLOW, "Render thread finished.");
    
    return 0;
}