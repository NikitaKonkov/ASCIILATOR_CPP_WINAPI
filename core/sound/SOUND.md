# Sound System – Stereo Spatialization & Clean API

## Overview

The sound system now features:
- Full 360° stereo spatialization for both tones and WAV files
- Clean separation: sound system handles only sound generation/playback; main program handles all input and logic
- Unified, easy-to-use API for tones and WAVs

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

## Usage Examples

```c
// Play a tone on the left for 3 seconds
sound_timer(1, 523.25, 1.0, 0.0, 3.0);
sound_angle(1, 270);

// Stop the tone
sound_kill(1);

// Play a delayed tone, then position it
sound_starter_timer(3, 440.0, 0.8, 0.0, 2.0, 0.5);
sound_angle(3, 315);

// Animate a spinning sound
int spin = sound_static(5, 880.0, 0.7, 0.0);
for (float a = 0; a < 360; a += 5) {
    sound_angle(spin, a);
    Sleep(50);
}

// Play a WAV on the left
int wav1 = sound_wav_timer(10, "explosion.wav", 1.0, 2.5);
sound_angle(100 + wav1, 270);

// Real-time control
sound_wav_set_amplitude(11, 1.0);    // Louder
sound_angle(100 + 11, 90);           // Move to right speaker

// Stop WAVs
sound_wav_kill(10);
sound_wav_kill_all();
```

---

## Architecture

- `sound_engine.c`: Audio device, threading, stereo output
- `sound_manager.c`: Voice allocation, mixing, angle calculations, API
- `sound_wav.c`: WAV loading/playback, stereo management
- `sound.h`: API definitions
- Main program: Handles all input, business logic, and calls sound API

---

## Key Features

- 360° stereo spatialization for all sounds
- Unified, simple API for tones and WAVs
- Real-time control of amplitude and position
- Clean separation of sound and input logic
- Efficient WAV file management and caching

---

This README now concisely and accurately describes your sound system’s capabilities, API, and usage.
