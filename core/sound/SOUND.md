# Sound System – Stereo Spatialization & Clean API

## Overview

The sound system is implemented as a C-style audio engine within the ASCIILATOR C++ project and features:
- Full 360° stereo spatialization for both tone## Key Features

- **360° Stereo Spatialization**: Full directional audio for immersive soundscapes
- **Unified API**: Simple C interface for both tones and WAV files
- **Real-time Control**: Dynamic amplitude and position adjustments
- **C++ Integration**: Seamlessly works with modern C++ manager classes
- **Thread-Safe**: Robust synchronization between C++ UI and C audio threads  
- **Efficient Asset Management**: Smart WAV file caching and memory handling
- **Cross-Platform Foundation**: Built on Windows WinMM with extensible architecture
- **Performance Optimized**: SIMD instructions and low-latency audio processing

---

## Getting Started

1. **Initialize**: Call `audio_init()` once at application startup
2. **Play Sounds**: Use `sound_timer()`, `sound_static()`, or WAV functions
3. **Position Audio**: Use `sound_angle()` for spatial positioning  
4. **Integration**: Call sound API from your C++ manager classes
5. **Cleanup**: Call `audio_shutdown()` before application exit

This sound system provides professional-grade audio capabilities while maintaining the simplicity needed for rapid game development within the ASCIILATOR C++ framework.iles
- Clean separation: sound system handles only sound generation/playback; main program handles all input and logic through C++ manager classes
- Unified, easy-to-use C API for tones and WAVs that integrates with the C++ application architecture
- Thread-safe audio processing with Windows WinMM API

---

## Core API

### Tone Functions

```c
// Play a tone for a set duration (auto-fades out)
int sound_timer(int id, double frequency, float amplitude, double phase, double duration_seconds);

// Play a tone until manually stopped
int sound_static(int id, double frequency, float amplitude, double phase);

// Play a tone after a delay, for a set duration
int sound_starter_timer(int id, double frequency, float amplitude, double phase, double duration_seconds, double start_delay_seconds);

// Play a tone after a delay, until manually stopped
int sound_starter_static(int id, double frequency, float amplitude, double phase, double start_delay_seconds);

// Stop a specific tone
void sound_kill(int id);

// Stop all tones
void sound_kill_all(void);
```

### Stereo Spatialization

```c
// Set 360° stereo position for any sound (tone or WAV)
// id: 0–15 for tones, 100–115 for WAVs (100 + wav_voice_id)
// angle: 0° = center, 90° = right, 180° = behind, 270° = left
void sound_angle(int id, float angle);
```

### WAV Audio Functions

```c
// Play WAV for a set duration
int sound_wav_timer(int id, const char* filename, float amplitude, double duration_seconds);

// Play WAV in a loop until stopped
int sound_wav_repeat(int id, const char* filename, float amplitude);

// Play WAV after a delay, for a set duration
int sound_wav_starter_timer(int id, const char* filename, float amplitude, double duration_seconds, double start_delay_seconds);

// Play WAV after a delay, loop until stopped
int sound_wav_starter_repeat(int id, const char* filename, float amplitude, double start_delay_seconds);

// Stop a specific WAV
void sound_wav_kill(int id);

// Stop all WAVs
void sound_wav_kill_all(void);

// Change amplitude of a playing WAV in real time
void sound_wav_set_amplitude(int id, float amplitude);

// WAV file management
bool load_wav_file(const char* filename);
void unload_wav_file(const char* filename);
void unload_all_wav_files(void);
```

---

## Integration with ASCIILATOR C++ Architecture

The sound system integrates seamlessly with the main ASCIILATOR application which uses C++ manager classes:

```cpp
// In your C++ application (main.cpp)
#include "sound/sound.hpp"

// Initialize audio system
audio_init();

// Use within C++ classes like ConsoleManager, InputManager, ClockManager
class GameEngine {
private:
    ConsoleManager console;
    InputManager input;
    ClockManager clock;
    
public:
    void playGameSound() {
        // Play explosion sound with spatial positioning
        int explosion = sound_wav_timer(5, "explosion.wav", 1.0, 2.0);
        sound_angle(100 + explosion, 90.0f); // Right speaker
    }
    
    void updateAmbientSound(float playerAngle) {
        // Dynamic ambient sound positioning
        sound_angle(1, playerAngle);
    }
};
```

### File Structure
```
core/
├── main.cpp              // Main C++ application with manager classes
├── console/
│   ├── console.hpp       // ConsoleManager class
│   └── console.cpp
├── input/
│   ├── input.hpp         // InputManager class  
│   └── input.cpp
├── clock/
│   ├── clock.hpp         // ClockManager class
│   └── clock.cpp
└── sound/
    ├── sound.hpp         // C-style sound API
    ├── sound.cpp         // C-style implementation
    └── SOUND.md          // This documentation
source/
└── sound/               // WAV audio files
    ├── explosion.wav
    ├── ambient.wav
    └── ...
```

---

## Usage Examples

```cpp
// Initialize the sound system (call once at startup)
audio_init();

// Play a tone on the left for 3 seconds
sound_timer(1, 523.25, 1.0, 0.0, 3.0);
sound_angle(1, 270.0f); // Left speaker

// Stop the tone
sound_kill(1);

// Play a delayed tone, then position it
sound_starter_timer(3, 440.0, 0.8, 0.0, 2.0, 0.5);
sound_angle(3, 315.0f); // Behind-left

// Animate a spinning sound (integrate with ClockManager for timing)
int spin = sound_static(5, 880.0, 0.7, 0.0);
for (float a = 0; a < 360; a += 5) {
    sound_angle(spin, a);
    Sleep(50); // Or use ClockManager.SyncClock() for better timing
}

// Play a WAV file positioned on the left
int wav1 = sound_wav_timer(10, "explosion.wav", 1.0, 2.5);
sound_angle(100 + wav1, 270.0f); // WAV IDs use 100 + wav_id offset

// Real-time control (useful with InputManager events)
sound_wav_set_amplitude(11, 1.0);    // Louder
sound_angle(100 + 11, 90.0f);        // Move to right speaker

// Stop WAVs
sound_wav_kill(10);
sound_wav_kill_all();

// Cleanup at shutdown
audio_shutdown();
```

---

## Architecture

The sound system is implemented as a C-style library within the larger C++ ASCIILATOR project:

### Sound System Components (C-style)
- **sound.cpp**: Audio device management, threading, stereo output, and WinMM integration
- **sound.hpp**: C API definitions, structs, and function declarations  
- **AudioSystem struct**: Global audio state management with thread synchronization

### Integration Layer
- **main.cpp**: C++ application using ConsoleManager, InputManager, ClockManager classes
- **Manager Classes**: Modern C++ classes that call the C-style sound API
- **Thread Safety**: Critical sections protect audio data between C++ and C layers

### Key Design Decisions
- Sound system remains C-style for direct WinMM API compatibility and performance
- C++ application layer provides modern object-oriented interface
- Clean separation allows sound engine to focus purely on audio processing
- WAV files stored in `source/sound/` directory for organized asset management

---

## Technical Details

### Audio Specifications
- **Sample Rate**: 44,100 Hz stereo
- **Bit Depth**: 16-bit signed integers
- **Buffer Size**: 2,205 samples (50ms latency)
- **Max Concurrent Sounds**: 16 tones + 16 WAV files
- **Spatial Audio**: Full 360° stereo positioning

### Thread Architecture
- **Audio Thread**: Dedicated Windows thread for real-time audio mixing
- **Main Thread**: C++ managers handle input/logic, call sound API
- **Synchronization**: Critical sections protect shared audio data
- **Performance**: SIMD optimizations for WAV format conversions

### WAV File Support
- **Formats**: 8-bit, 16-bit, 24-bit, 32-bit PCM
- **Channels**: Mono and stereo
- **Sample Rates**: Any rate (auto-converted to 44.1kHz)
- **Caching**: Automatic loading and memory management
- **Path**: WAV files loaded from `source/sound/` directory

---

## Key Features

- 360° stereo spatialization for all sounds
- Unified, simple API for tones and WAVs
- Real-time control of amplitude and position
- Clean separation of sound and input logic
- Efficient WAV file management and caching

---

This README now concisely and accurately describes your sound system’s capabilities, API, and usage.
