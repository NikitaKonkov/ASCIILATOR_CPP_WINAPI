# ThreadManager – Multi-Threading Abstraction for ASCIILATOR

## Overview

The **ThreadManager** provides a clean, high-level abstraction for multi-threaded application architecture:
- **Thread Lifecycle Management** – Simplified creation, monitoring, and cleanup of worker threads
- **Console Thread Coordination** – 60Hz input processing with synchronized exit handling
- **Window Thread Management** – 5Hz heartbeat updates with GUI message loop integration
- **Global Exit Synchronization** – Thread-safe shutdown coordination using volatile flags
- **Error Handling** – Comprehensive thread creation and cleanup with proper resource management
- **Win32 Integration** – Native Windows threading using CreateThread and WaitForMultipleObjects

File locations:
- `core/thread/thread.hpp` – ThreadManager class definition and public API
- `core/thread/thread.cpp` – Implementation with thread procedures and lifecycle management

---

## Quick Start

```cpp
#include "thread.hpp"

ThreadManager threadManager(&g_shouldExit);

// Start both threads
threadManager.StartConsoleThread();
threadManager.StartWindowThread();

// Wait for clean shutdown
threadManager.WaitForThreadsToFinish();
```

## API Reference

### Core Thread Management
```cpp
class ThreadManager {
public:
    ThreadManager(volatile bool* globalExitFlag);  // Initialize with exit coordination flag
    ~ThreadManager();                              // Cleanup resources and handles
    
    bool StartConsoleThread();                     // Start 60Hz input processing thread
    bool StartWindowThread();                      // Start 5Hz window heartbeat thread
    void WaitForThreadsToFinish();                 // Block until all threads complete
    
private:
    // Internal thread procedures
    static DWORD WINAPI ConsoleThreadProc(LPVOID lpParam);
    static DWORD WINAPI WindowThreadProc(LPVOID lpParam);
};
```

### Constructor Parameters
```cpp
ThreadManager(volatile bool* globalExitFlag)
```
- **globalExitFlag**: Pointer to application-wide exit coordination flag
- Must be `volatile bool*` to ensure thread-safe visibility across threads
- Typically points to global `g_shouldExit` variable

### Return Values
- **StartConsoleThread()**: Returns `true` if thread created successfully, `false` on error
- **StartWindowThread()**: Returns `true` if thread created successfully, `false` on error
- **WaitForThreadsToFinish()**: Blocks until all active threads terminate (void return)

---

## Thread Architecture

### Console Thread (60Hz Processing)
```cpp
// Internal behavior - automatically managed by ThreadManager
while (!*exitFlag) {
    // Process keyboard input via InputManager
    // Handle escape key detection
    // Coordinate with other application components
    Sleep(16);  // ~60Hz update rate
}
```

**Purpose**: High-frequency input processing and application control
**Frequency**: ~60Hz (16ms sleep intervals)  
**Responsibilities**:
- Keyboard input polling and processing
- Exit condition detection (ESC key)
- Real-time user interaction handling

### Window Thread (5Hz Updates)
```cpp
// Internal behavior - automatically managed by ThreadManager
while (!*exitFlag) {
    // Window message processing via WindowManager
    // Heartbeat status updates
    // GUI responsiveness maintenance
    Sleep(200);  // ~5Hz update rate
}
```

**Purpose**: GUI responsiveness and periodic status updates
**Frequency**: ~5Hz (200ms sleep intervals)
**Responsibilities**:
- Win32 message loop processing
- Window heartbeat and status display
- GUI thread coordination

---

## Usage Patterns

### Basic Application Structure
```cpp
#include "thread.hpp"
#include <iostream>

// Global exit flag for thread coordination
volatile bool g_shouldExit = false;

int main() {
    // Initialize ThreadManager with exit flag
    ThreadManager threadManager(&g_shouldExit);
    
    // Start application threads
    if (!threadManager.StartConsoleThread()) {
        std::cerr << "Failed to start console thread!" << std::endl;
        return 1;
    }
    
    if (!threadManager.StartWindowThread()) {
        std::cerr << "Failed to start window thread!" << std::endl;
        return 1;
    }
    
    std::cout << "Both console and window threads started successfully!" << std::endl;
    std::cout << "Press ESC to exit the application." << std::endl;
    
    // Wait for threads to complete
    threadManager.WaitForThreadsToFinish();
    
    std::cout << "Application shutdown complete." << std::endl;
    return 0;
}
```

### Thread-Safe Exit Coordination
```cpp
// Global exit flag must be volatile for thread visibility
volatile bool g_shouldExit = false;

// In any thread, trigger application shutdown:
g_shouldExit = true;

// All threads will detect the flag change and begin cleanup
// WaitForThreadsToFinish() will unblock when all threads exit
```

### Error Handling
```cpp
ThreadManager threadManager(&g_shouldExit);

// Check thread creation success
if (!threadManager.StartConsoleThread()) {
    // Handle console thread creation failure
    // Application should not continue without input processing
}

if (!threadManager.StartWindowThread()) {
    // Handle window thread creation failure
    // May continue with console-only operation
}

// ThreadManager destructor automatically cleans up handles
// No manual cleanup required
```

---

## Integration Requirements

### Global Dependencies
The ThreadManager requires these global components to be properly initialized:

```cpp
// Required for console thread operation
extern InputManager inputManager;   // Keyboard input processing
extern ConsoleManager console;      // Console output management

// Required for window thread operation  
extern WindowManager windowManager; // Window and GUI management
extern ClockManager clockManager;   // Timing and heartbeat functionality
```

### Thread-Safe Components
All integrated components must be thread-safe for concurrent access:
- **InputManager**: Thread-safe keyboard state querying
- **ConsoleManager**: Thread-safe console output operations
- **WindowManager**: Thread-safe window message processing
- **ClockManager**: Thread-safe timing operations

### Volatile Flag Management
```cpp
// Global exit flag MUST be volatile for proper thread coordination
volatile bool g_shouldExit = false;

// Pass by reference to ThreadManager
ThreadManager threadManager(&g_shouldExit);

// Any thread can set the flag to trigger shutdown
g_shouldExit = true;  // All threads will detect and begin cleanup
```

---

## Performance Characteristics

### Thread Update Frequencies
| Thread | Frequency | Sleep Time | Purpose |
|--------|-----------|------------|---------|
| Console | ~60Hz | 16ms | Real-time input processing |
| Window | ~5Hz | 200ms | GUI updates and heartbeat |

### Resource Usage
- **Memory**: Minimal overhead (~8 bytes per thread handle)
- **CPU**: Negligible when idle (sleep-based scheduling)  
- **Handles**: 2 thread handles (automatically cleaned up)

### Synchronization Model
- **Exit Flag**: Volatile boolean for lock-free coordination
- **Thread Waiting**: WaitForMultipleObjects for efficient blocking
- **No Mutexes**: Lock-free design minimizes contention

---

## Advanced Usage

### Custom Thread Frequencies
Modify sleep intervals in thread procedures for different update rates:
```cpp
// For higher console responsiveness (120Hz):
Sleep(8);   // ~120Hz in ConsoleThreadProc

// For more frequent window updates (10Hz):
Sleep(100); // ~10Hz in WindowThreadProc
```

### Thread Priority Adjustment
```cpp
// In thread procedures, adjust priority if needed:
SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
```

### Extended Exit Conditions
```cpp
// Add custom exit conditions in thread procedures:
while (!*exitFlag && !customExitCondition) {
    // Thread processing...
}
```

---

## Troubleshooting

### Common Issues

**Thread Creation Fails**
- Check system resources and thread limits
- Verify globalExitFlag pointer is valid
- Ensure required components are initialized

**Threads Don't Exit Cleanly**  
- Verify volatile bool* flag is being set correctly
- Check that all component cleanup is thread-safe
- Monitor for infinite loops in thread procedures

**Application Hangs on Exit**
- Ensure WaitForThreadsToFinish() is called
- Verify exit flag is set before waiting
- Check for deadlocks in component cleanup

### Debug Output
Enable debug output in thread procedures for troubleshooting:
```cpp
std::cout << "Console thread starting..." << std::endl;
std::cout << "Console thread finished." << std::endl;
```

### Performance Issues
- Monitor thread CPU usage with Task Manager
- Adjust sleep intervals if threads consume too much CPU
- Consider component initialization order for optimal startup

---

## Design Philosophy

The ThreadManager follows these architectural principles:

1. **Separation of Concerns**: Threading logic isolated from application logic
2. **Simple Interface**: Minimal API surface for ease of use  
3. **Resource Management**: Automatic cleanup of system resources
4. **Thread Safety**: Lock-free coordination using volatile flags
5. **Error Resilience**: Graceful degradation on thread creation failures
6. **Performance**: Sleep-based scheduling for minimal CPU overhead

This design enables clean, maintainable multi-threaded applications with minimal complexity while providing robust thread lifecycle management for the ASCIILATOR engine.