# ThreadManager Usage Examples

## Basic Thread Creation with IDs

```cpp
#include "thread/thread.hpp"

volatile bool g_shouldExit = false;
ThreadManager threadManager(&g_shouldExit);

// Create threads with custom IDs from main.cpp
threadManager.CreateThread("main_input", ThreadType::CONSOLE_THREAD);
threadManager.CreateThread("ui_window", ThreadType::WINDOW_THREAD);
threadManager.CreateThread("background_console", ThreadType::CONSOLE_THREAD);

// Wait for all threads to finish
threadManager.WaitForThreadsToFinish();
```

## Advanced Thread Management

```cpp
// Create multiple threads with descriptive IDs
threadManager.CreateThread("player_input", ThreadType::CONSOLE_THREAD);
threadManager.CreateThread("game_window", ThreadType::WINDOW_THREAD);
threadManager.CreateThread("debug_console", ThreadType::CONSOLE_THREAD);
threadManager.CreateThread("status_display", ThreadType::WINDOW_THREAD);

// Check if specific threads are running
if (threadManager.IsThreadRunning("player_input")) {
    std::cout << "Player input thread is active" << std::endl;
}

// Wait for a specific thread
threadManager.WaitForThread("game_window");

// Kill specific thread if needed
threadManager.KillThread("debug_console");
```

## Thread Lifecycle Management

```cpp
// All lifecycle operations are handled by ThreadManager:

// 1. CREATION - Managed from main.cpp with IDs
threadManager.CreateThread("worker_1", ThreadType::CONSOLE_THREAD);
threadManager.CreateThread("worker_2", ThreadType::WINDOW_THREAD);

// 2. SYNCHRONIZATION - Handled by ThreadManager
threadManager.WaitForThreadsToFinish();  // Wait for all
threadManager.WaitForThread("worker_1"); // Wait for specific

// 3. CLEANUP - Automatic via destructor or manual
threadManager.Cleanup();                 // Manual cleanup
// Or automatic cleanup when ThreadManager goes out of scope

// 4. EMERGENCY TERMINATION - When threads don't exit cleanly
threadManager.KillThread("stuck_thread");     // Kill specific
threadManager.KillAllThreads();               // Kill all
```

## Output Example

```
Thread 'player_input' created successfully (ID: 12345)
Thread 'game_window' created successfully (ID: 12346)
Thread 'debug_console' created successfully (ID: 12347)
Both console and window threads started successfully!
Press ESC in either thread to exit the application.
Escape key pressed. Exiting console thread.
Thread 'player_input' finished.
ThreadManager initiating shutdown...
Application shutdown complete.
```

## Benefits of ID-Based System

1. **Flexible Creation**: Create threads from main.cpp with meaningful names
2. **Easy Tracking**: Monitor specific threads by their ID
3. **Selective Control**: Kill, wait for, or check specific threads
4. **Clean Architecture**: ThreadManager handles all lifecycle complexity
5. **Scalable**: Easy to add new thread types without changing core logic