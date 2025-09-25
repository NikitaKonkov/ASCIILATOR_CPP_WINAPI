# Sound Thread Usage Guide

## 🎵 Simple Sound Thread Controls

Your ASCIILATOR application now includes a dedicated sound thread with simple keyboard controls:

### 🎹 **Keyboard Controls**

| Key | Action | Description |
|-----|--------|-------------|
| **P** | Play WAV | Cycles through available WAV files (ahem_x.wav → air_raid.wav → airplane_chime_x.wav) |
| **S** | Stop All | Stops all currently playing sounds (both WAV and tones) |
| **T** | Play Tone | Plays a 440Hz tone for 2 seconds |
| **ESC** | Exit App | Exits the entire application (from console thread) |

### 🚀 **How It Works**

1. **Sound Thread**: Runs independently at ~60 FPS monitoring for key presses
2. **Audio System**: Automatically initializes when sound thread starts
3. **WAV Loading**: Automatically loads all WAV files from `source/sound/` directory
4. **Clean Shutdown**: Stops all sounds and shuts down audio system on exit

### 📁 **Available WAV Files**

The system automatically loads these files from `source/sound/`:
- `ahem_x.wav` - Short voice sound
- `air_raid.wav` - Air raid siren
- `airplane_chime_x.wav` - Airplane chime sound

### 🔧 **Technical Details**

- **Thread ID**: `sound_audio` (managed by ThreadManager)
- **Audio Format**: 44.1kHz stereo, 16-bit
- **Performance**: SIMD-optimized WAV processing
- **Memory**: Smart caching system for loaded WAV files

### 💡 **Usage Examples**

```cpp
// The sound thread is automatically created in main.cpp:
threadManager.CreateThread("sound_audio", ThreadType::SOUND_THREAD);

// No additional setup required - just run the application and use the controls!
```

### 🎛️ **Real-Time Output**

When you use the controls, you'll see console output like:
```
Playing WAV: air_raid.wav (ID: 11)
All sounds stopped!
Playing tone: 440Hz (ID: 5)
```

The sound thread demonstrates perfect integration with your existing ThreadManager architecture - it creates its own audio resources, handles input independently, and cleans up properly on shutdown! 🎵