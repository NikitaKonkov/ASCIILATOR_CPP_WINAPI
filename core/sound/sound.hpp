#if !defined(SOUND_HPP)
#define SOUND_HPP

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SAMPLE_RATE 44100
#define AMPLITUDE 15000
#define BUFFER_SIZE 2205
#define PI 3.14159265358979323846
#define MAX_SOUNDS 16
#define MAX_WAV_SOUNDS 16
#define FADE_SAMPLES 220

#define ANGLE_TO_RADIANS(angle) ((angle) * PI / 180.0)
#define RADIANS_TO_ANGLE(radians) ((radians) * 180.0 / PI)

#pragma comment(lib, "winmm.lib")

typedef struct {
    double frequency;
    double phase;
    float amplitude;
    float angle;
    float left_amp;
    float right_amp;
    bool active;
    int fade_state;
    int fade_counter;
    int fade_duration;
    int timer_samples;
    int timer_counter;
    int delay_samples;
    int delay_counter;
    bool is_timed_after_delay;
    double delayed_duration_seconds;
    int sound_index;
    float reverb_amount;
    float reverb_decay;
    short reverb_buffer[8820];
    int reverb_index;
} Sound;

typedef struct {
    short* data;
    int sample_count;
    int channels;
    int sample_rate;
    char filename[256];
    bool loaded;
} WavData;

typedef struct {
    WavData* wav_data;
    int current_position;
    float fractional_position;
    float amplitude;
    float angle;
    float left_amp;
    float right_amp;
    bool active;
    bool repeat;
    int fade_state;
    int fade_counter;
    int fade_duration;
    int timer_samples;
    int timer_counter;
    int delay_samples;
    int delay_counter;
    bool is_timed_after_delay;
    double delayed_duration_seconds;
    int sound_index;
    float reverb_amount;
    float reverb_decay;
    short reverb_buffer[8820];
    int reverb_index;
} WavSound;

typedef struct {
    HWAVEOUT hWaveOut;
    WAVEHDR waveHeaders[3];
    short audioBuffers[3][BUFFER_SIZE * 2];
    int currentBuffer;
    Sound sounds[MAX_SOUNDS];
    WavSound wav_sounds[MAX_WAV_SOUNDS];
    WavData wav_cache[32];
    int wav_cache_count;
    bool initialized;
    bool running;
    HANDLE audioThread;
    CRITICAL_SECTION soundLock;
    CRITICAL_SECTION wavLock;
} AudioSystem;

extern AudioSystem g_audioSystem;

// Sound Manager Class
class SoundManager {
public:
    // Audio System Methods
    bool AudioInit(void);
    void AudioShutdown(void);

    // Sound Generation Methods
    int SoundTimer(int id, double frequency, float amplitude, double phase, double duration_seconds);
    int SoundStatic(int id, double frequency, float amplitude, double phase);
    int SoundStarterTimer(int id, double frequency, float amplitude, double phase, double duration_seconds, double start_delay_seconds);
    int SoundStarterStatic(int id, double frequency, float amplitude, double phase, double start_delay_seconds);
    void SoundKill(int id);
    void SoundKillAll(void);

    // Sound Effects Methods
    void SoundAngle(int id, float angle);
    void SoundReverb(int id, float amount, float decay);

    // Audio Control Methods
    int AudioPlayTone(double frequency, float gain);
    void AudioStopSound(int sound_id);
    void AudioStopAllSounds(void);

    // WAV File Methods
    int SoundWavTimer(int id, const char* filename, float amplitude, double duration_seconds);
    int SoundWavRepeat(int id, const char* filename, float amplitude);
    int SoundWavStarterTimer(int id, const char* filename, float amplitude, double duration_seconds, double start_delay_seconds);
    int SoundWavStarterRepeat(int id, const char* filename, float amplitude, double start_delay_seconds);
    void SoundWavKill(int id);
    void SoundWavKillAll(void);
    void SoundWavSetAmplitude(int id, float amplitude);

    // WAV File Management Methods
    bool LoadWavFile(const char* filename);
    void UnloadWavFile(const char* filename);
    void UnloadAllWavFiles(void);
};

#endif // SOUND_HPP