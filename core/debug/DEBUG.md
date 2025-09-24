# ASCIILATOR Debug System

## Overview

The ASCIILATOR Debug System is a comprehensive testing framework for all system components. It provides modular debugging capabilities with easy-to-use interfaces for testing input, display, clock, and sound systems.

---

## Architecture

### Core Components

- **DebugManager Class**: Main debug controller with mode management
- **Debug Modes**: Separate testing modes for different system components
- **Modular Design**: Each system component can be tested independently
- **Global Instance**: `g_debugManager` for easy access across the application

### File Structure
```
core/
├── debug/
│   ├── debug.hpp         // DebugManager class and enums
│   ├── debug.cpp         // Implementation of all debug functionality
│   └── DEBUG.md          // This documentation
├── main_simple.cpp       // Simplified main using debug system
└── main.cpp             // Original full-featured main (legacy)
```

---

## Debug Modes

### 1. FPS Adjustment Mode (DEBUG_MODE_FPS_ADJUSTMENT)
**Hotkey**: `F`
- Test different frame rates for the main loop
- **Controls**:
  - `1`: Set to 30 FPS
  - `2`: Set to 60 FPS  
  - `3`: Set to 120 FPS
- **Purpose**: Performance testing and timing analysis

### 2. Sound Testing Mode (DEBUG_MODE_SOUND_TESTING)
**Hotkey**: `S`
- Test stereo spatialization and audio playback
- **Controls**:
  - `1-6`: Musical tones with different spatial positions
  - `7-9`: WAV file playback (ahem, air_raid, airplane)
  - `0`: Spinning stereo sound demo
- **Purpose**: Audio system validation and spatial audio testing

### 3. Input Testing Mode (DEBUG_MODE_INPUT_TESTING)
**Hotkey**: `T`
- Real-time keyboard and mouse monitoring
- **Features**:
  - Live key press display
  - Mouse position and button tracking
  - Key combination detection
  - Mouse movement detection
- **Purpose**: Input system reliability and responsiveness testing

### 4. Display Testing Mode (DEBUG_MODE_DISPLAY_TESTING)
**Hotkey**: `D`
- Visual display system testing
- **Controls**:
  - `1`: Color testing (Red, Green, Blue)
  - `2`: Box drawing test
  - `3`: Line drawing test
  - `4`: Animation frame test
- **Purpose**: Display rendering and ANSI color validation

---

## Usage

### Basic Usage

```cpp
#include "debug/debug.hpp"

int main() {
    DisplayManager display;
    InputManager input;
    ClockManager clock;
    
    // Initialize debug system
    g_debugManager.Initialize(&display, &input, &clock);
    
    // Show welcome screen
    g_debugManager.DisplayWelcomeScreen();
    
    // Run debug loop
    g_debugManager.RunDebugLoop();
    
    return 0;
}
```

### Advanced Usage

```cpp
// Test specific components
DEBUG_TestInput();    // Switch to input testing
DEBUG_TestSound();    // Switch to sound testing
DEBUG_TestDisplay();  // Switch to display testing
DEBUG_TestClock();    // Switch to FPS testing
DEBUG_TestAll();      // Cycle through all tests

// Manual mode switching
g_debugManager.SetMode(DEBUG_MODE_SOUND_TESTING);

// Toggle detailed information
g_debugManager.ToggleDetailedInfo();
```

---

## Global Controls

| Key | Action |
|-----|--------|
| `F` | Switch to FPS Adjustment Mode |
| `S` | Switch to Sound Testing Mode |
| `T` | Switch to Input Testing Mode |
| `D` | Switch to Display Testing Mode |
| `I` | Toggle detailed system information |
| `ESC` | Exit debug system |

---

## Integration with Main Application

### Option 1: Use Simplified Main
Replace your main.cpp with main_simple.cpp for a clean debug-only application.

### Option 2: Add Debug Module
Keep your existing main.cpp and add debug capabilities:

```cpp
#include "debug/debug.hpp"

// In your application
if (/* debug mode requested */) {
    g_debugManager.Initialize(&display, &input, &clock);
    g_debugManager.RunDebugLoop();
}
```

### Option 3: Selective Testing
Use individual debug functions for specific testing:

```cpp
// Test only sound system
g_debugManager.Initialize(&display, &input, &clock);
DEBUG_TestSound();
```

---

## Features

### Real-time Monitoring
- **Performance**: FPS, frame timing, system uptime
- **Input**: Key states, mouse position, button presses
- **Audio**: Sound playback status, spatial positioning
- **Display**: Rendering tests, color validation

### Comprehensive Testing
- **Sound System**: 360° spatial audio, WAV playback, tone generation
- **Input System**: Keyboard/mouse responsiveness, combination detection
- **Display System**: ANSI colors, cursor positioning, text formatting
- **Clock System**: Multi-clock timing, FPS management

### User-Friendly Interface
- **Mode Indicators**: Clear visual feedback of current test mode
- **Real-time Feedback**: Instant response to user actions
- **Detailed Information**: Optional detailed system statistics
- **Clean Exit**: Proper cleanup and final statistics

---

## Best Practices

1. **Initialize Once**: Call `g_debugManager.Initialize()` only once per session
2. **Proper Cleanup**: Always call `g_debugManager.Shutdown()` before exit
3. **Mode Switching**: Use hotkeys to switch between test modes efficiently
4. **Error Checking**: Check return values from Initialize() method
5. **Resource Management**: Debug system handles all audio and resource cleanup

---

## Troubleshooting

### Common Issues
- **Audio Not Working**: Ensure WAV files exist in `source/sound/` directory
- **Input Not Responding**: Check if input manager is properly initialized
- **Display Issues**: Verify console supports ANSI escape sequences
- **Performance Problems**: Use FPS mode to identify timing bottlenecks

### Debug Output
The debug system provides real-time status information for all components, making it easy to identify and resolve issues quickly.

---

This modular debug system allows for comprehensive testing of all ASCIILATOR components while maintaining clean, readable code that can be easily extended for future testing needs.