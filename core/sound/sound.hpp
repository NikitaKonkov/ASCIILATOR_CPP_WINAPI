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

bool audio_init(void);
void audio_shutdown(void);

int sound_timer(int id, double frequency, float amplitude, double phase, double duration_seconds);
int sound_static(int id, double frequency, float amplitude, double phase);
int sound_starter_timer(int id, double frequency, float amplitude, double phase, double duration_seconds, double start_delay_seconds);
int sound_starter_static(int id, double frequency, float amplitude, double phase, double start_delay_seconds);
void sound_kill(int id);
void sound_kill_all(void);

void sound_angle(int id, float angle);
void sound_reverb(int id, float amount, float decay);

int audio_play_tone(double frequency, float gain);
void audio_stop_sound(int sound_id);
void audio_stop_all_sounds(void);

int sound_wav_timer(int id, const char* filename, float amplitude, double duration_seconds);
int sound_wav_repeat(int id, const char* filename, float amplitude);
int sound_wav_starter_timer(int id, const char* filename, float amplitude, double duration_seconds, double start_delay_seconds);
int sound_wav_starter_repeat(int id, const char* filename, float amplitude, double start_delay_seconds);
void sound_wav_kill(int id);
void sound_wav_kill_all(void);
void sound_wav_set_amplitude(int id, float amplitude);

bool load_wav_file(const char* filename);
void unload_wav_file(const char* filename);
void unload_all_wav_files(void);

#endif // SOUND_HPP