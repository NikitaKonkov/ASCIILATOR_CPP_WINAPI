#include "sound.hpp"

AudioSystem g_audioSystem = {0};

// Forward declarations for functions used in audio_init and audio_shutdown
extern void generate_mixed_audio(short* buffer, int buffer_size);
int sound_static(int id, double frequency, float amplitude, double phase);
void sound_angle(int id, float angle);
void audio_stop_all_sounds(void);
void sound_wav_kill_all(void);
void unload_all_wav_files(void);

static DWORD WINAPI audio_thread_proc(LPVOID lpParam) {
    printf("Audio thread started\n");
    
    const int NUM_BUFFERS = 3;
    
    while (g_audioSystem.running) {
        for (int i = 0; i < NUM_BUFFERS; i++) {
            WAVEHDR* header = &g_audioSystem.waveHeaders[i];
            
            if (header->dwFlags & WHDR_DONE || !(header->dwFlags & WHDR_PREPARED)) {
                generate_mixed_audio(g_audioSystem.audioBuffers[i], BUFFER_SIZE);
                
                if (header->dwFlags & WHDR_PREPARED) {
                    waveOutUnprepareHeader(g_audioSystem.hWaveOut, header, sizeof(WAVEHDR));
                }
                
                header->dwFlags = 0;
                if (waveOutPrepareHeader(g_audioSystem.hWaveOut, header, sizeof(WAVEHDR)) == MMSYSERR_NOERROR) {
                    waveOutWrite(g_audioSystem.hWaveOut, header, sizeof(WAVEHDR));
                }
            }
        }
        
        Sleep(5);
    }
    
    printf("Audio thread stopped\n");
    return 0;
}

bool audio_init(void) {
    if (g_audioSystem.initialized) {
        return true;
    }
    
    printf("Initializing audio system...\n");
    
    memset(&g_audioSystem, 0, sizeof(AudioSystem));
    
    InitializeCriticalSection(&g_audioSystem.soundLock);
    InitializeCriticalSection(&g_audioSystem.wavLock);
    
    WAVEFORMATEX waveFormat;
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = SAMPLE_RATE;
    waveFormat.nAvgBytesPerSec = SAMPLE_RATE * 4;
    waveFormat.nBlockAlign = 4;
    waveFormat.wBitsPerSample = 16;
    waveFormat.cbSize = 0;
    
    MMRESULT result = waveOutOpen(&g_audioSystem.hWaveOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
    
    if (result != MMSYSERR_NOERROR) {
        printf("Failed to open audio device. Error: %d\n", result);
        DeleteCriticalSection(&g_audioSystem.soundLock);
        DeleteCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    const int NUM_BUFFERS = 3;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        g_audioSystem.waveHeaders[i].lpData = (LPSTR)g_audioSystem.audioBuffers[i];
        g_audioSystem.waveHeaders[i].dwBufferLength = BUFFER_SIZE * 2 * sizeof(short);
        g_audioSystem.waveHeaders[i].dwFlags = 0;
        g_audioSystem.waveHeaders[i].dwLoops = 0;
    }
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        g_audioSystem.sounds[i].active = false;
        g_audioSystem.sounds[i].angle = 0.0f;
        g_audioSystem.sounds[i].left_amp = 1.0f;
        g_audioSystem.sounds[i].right_amp = 1.0f;
        g_audioSystem.sounds[i].fade_duration = FADE_SAMPLES;
        g_audioSystem.sounds[i].delay_samples = 0;
        g_audioSystem.sounds[i].delay_counter = 0;
        g_audioSystem.sounds[i].is_timed_after_delay = false;
        g_audioSystem.sounds[i].delayed_duration_seconds = 0.0;
        g_audioSystem.sounds[i].sound_index = i;
        g_audioSystem.sounds[i].reverb_amount = 0.0f;
        g_audioSystem.sounds[i].reverb_decay = 0.5f;
        g_audioSystem.sounds[i].reverb_index = 0;
        memset(g_audioSystem.sounds[i].reverb_buffer, 0, sizeof(g_audioSystem.sounds[i].reverb_buffer));
    }
    
    for (int i = 0; i < MAX_WAV_SOUNDS; i++) {
        g_audioSystem.wav_sounds[i].active = false;
        g_audioSystem.wav_sounds[i].current_position = 0;
        g_audioSystem.wav_sounds[i].fractional_position = 0.0f;
        g_audioSystem.wav_sounds[i].angle = 0.0f;
        g_audioSystem.wav_sounds[i].left_amp = 1.0f;
        g_audioSystem.wav_sounds[i].right_amp = 1.0f;
        g_audioSystem.wav_sounds[i].fade_duration = FADE_SAMPLES;
        g_audioSystem.wav_sounds[i].delay_samples = 0;
        g_audioSystem.wav_sounds[i].delay_counter = 0;
        g_audioSystem.wav_sounds[i].is_timed_after_delay = false;
        g_audioSystem.wav_sounds[i].delayed_duration_seconds = 0.0;
        g_audioSystem.wav_sounds[i].sound_index = i;
        g_audioSystem.wav_sounds[i].reverb_amount = 0.0f;
        g_audioSystem.wav_sounds[i].reverb_decay = 0.5f;
        g_audioSystem.wav_sounds[i].reverb_index = 0;
        memset(g_audioSystem.wav_sounds[i].reverb_buffer, 0, sizeof(g_audioSystem.wav_sounds[i].reverb_buffer));
    }
    
    g_audioSystem.wav_cache_count = 0;
    for (int i = 0; i < 32; i++) {
        g_audioSystem.wav_cache[i].loaded = false;
        g_audioSystem.wav_cache[i].data = NULL;
    }
    
    g_audioSystem.initialized = true;
    g_audioSystem.running = true;
    
    g_audioSystem.audioThread = CreateThread(NULL, 0, audio_thread_proc, NULL, 0, NULL);
    if (g_audioSystem.audioThread == NULL) {
        printf("Failed to create audio thread\n");
        waveOutClose(g_audioSystem.hWaveOut);
        DeleteCriticalSection(&g_audioSystem.soundLock);
        DeleteCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    printf("Audio system initialized successfully!\n");
    printf("Sample Rate: %d Hz, Buffer Size: %d samples\n", SAMPLE_RATE, BUFFER_SIZE);
    printf("Max Sounds: %d, Max WAV Sounds: %d, Fade Duration: %d samples\n", MAX_SOUNDS, MAX_WAV_SOUNDS, FADE_SAMPLES);
    
    sound_static(0, 20.0, 0.01f, 0.0);
    sound_angle(0, 0);
    printf("Permanent carrier tone active (Sound 0 - DO NOT KILL)\n");
    return true;
}

void audio_shutdown(void) {
    if (!g_audioSystem.initialized) {
        return;
    }
    
    printf("Shutting down audio system...\n");
    
    g_audioSystem.running = false;
    
    if (g_audioSystem.audioThread != NULL) {
        WaitForSingleObject(g_audioSystem.audioThread, 1000);
        CloseHandle(g_audioSystem.audioThread);
        g_audioSystem.audioThread = NULL;
    }
    
    audio_stop_all_sounds();
    sound_wav_kill_all();
    
    unload_all_wav_files();
    
    waveOutReset(g_audioSystem.hWaveOut);
    
    const int NUM_BUFFERS = 3;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (g_audioSystem.waveHeaders[i].dwFlags & WHDR_PREPARED) {
            waveOutUnprepareHeader(g_audioSystem.hWaveOut, &g_audioSystem.waveHeaders[i], sizeof(WAVEHDR));
        }
    }
    
    waveOutClose(g_audioSystem.hWaveOut);
    
    DeleteCriticalSection(&g_audioSystem.soundLock);
    DeleteCriticalSection(&g_audioSystem.wavLock);
    
    g_audioSystem.initialized = false;
    
    printf("Audio system shut down.\n");
}




















#define SINE_TABLE_SIZE 1024
static float sine_table[SINE_TABLE_SIZE];
static bool sine_table_initialized = false;

static void init_sine_table() {
    if (sine_table_initialized) return;
    
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        sine_table[i] = (float)sin(2.0 * PI * i / SINE_TABLE_SIZE);
    }
    sine_table_initialized = true;
}

static inline float fast_sin(double phase) {
    phase = phase / (2.0 * PI);
    phase = phase - floor(phase);
    
    int index = (int)(phase * SINE_TABLE_SIZE);
    index = index & (SINE_TABLE_SIZE - 1);
    
    return sine_table[index];
}

static inline void calculate_stereo_amplitudes(float angle, float* left_amp, float* right_amp) {
    angle = fmodf(angle, 360.0f);
    if (angle < 0) angle += 360.0f;
    
    // Your desired mapping: 0°/360° = behind, 90° = right, 180° = front, 270° = left
    
    float rad = ANGLE_TO_RADIANS(angle);
    
    // Calculate left-right positioning using sine 
    // sin(90°) = 1 (right side), sin(270°) = -1 (left side)
    float lr_component = sinf(rad);  
    
    // Calculate front-back positioning using cosine
    // cos(180°) = -1 (front), cos(0°) = 1 (back)  
    float fb_component = cosf(rad);  
    
    // Apply front-back attenuation (sounds behind are quieter)
    float distance_factor = (fb_component > 0) ? 0.3f : 1.0f;
    
    // Calculate stereo amplitudes
    // When lr_component > 0 (right side), favor right ear
    // When lr_component < 0 (left side), favor left ear  
    *right_amp = (0.5f + lr_component * 0.5f) * distance_factor;  // More right when lr_component is positive
    *left_amp = (0.5f - lr_component * 0.5f) * distance_factor;   // More left when lr_component is negative
    
    if (*left_amp < 0) *left_amp = 0;
    if (*right_amp < 0) *right_amp = 0;
    if (*left_amp > 1) *left_amp = 1;
    if (*right_amp > 1) *right_amp = 1;
}

static void init_sound_common(Sound* sound, double frequency, float amplitude, double phase) {
    sound->frequency = frequency;
    sound->phase = phase;
    sound->amplitude = amplitude;
    sound->angle = 0.0f;
    sound->left_amp = 1.0f;
    sound->right_amp = 1.0f;
    sound->active = true;
    sound->fade_counter = 0;
    sound->fade_duration = FADE_SAMPLES;
    sound->timer_samples = 0;
    sound->timer_counter = 0;
    sound->delay_samples = 0;
    sound->delay_counter = 0;
    sound->is_timed_after_delay = false;
    sound->delayed_duration_seconds = 0.0;
    sound->reverb_amount = 0.0f;
    sound->reverb_decay = 0.5f;
    sound->reverb_index = 0;
    memset(sound->reverb_buffer, 0, sizeof(sound->reverb_buffer));
}

void generate_mixed_audio(short* buffer, int buffer_size) {
    init_sine_table();
    
    memset(buffer, 0, buffer_size * 2 * sizeof(short));
    
    Sound active_sounds[MAX_SOUNDS];
    int active_count = 0;
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    for (int v = 0; v < MAX_SOUNDS; v++) {
        if (g_audioSystem.sounds[v].active) {
            active_sounds[active_count] = g_audioSystem.sounds[v];
            active_sounds[active_count].sound_index = v;
            active_count++;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    
    for (int v = 0; v < active_count; v++) {
        Sound* sound = &active_sounds[v];
        
        if (sound->fade_state == 4) {
            sound->delay_counter += buffer_size;
            if (sound->delay_counter >= sound->delay_samples) {
                if (sound->is_timed_after_delay) {
                    sound->fade_state = 3;
                    sound->timer_samples = (int)(sound->delayed_duration_seconds * SAMPLE_RATE);
                    sound->timer_counter = 0;
                } else {
                    sound->fade_state = 0;
                }
                sound->fade_counter = 0;
                sound->fade_duration = FADE_SAMPLES;
            } else {
                continue;
            }
        }
        
        double phase_increment = 2.0 * PI * sound->frequency / SAMPLE_RATE;
        
        calculate_stereo_amplitudes(sound->angle, &sound->left_amp, &sound->right_amp);
        
        for (int i = 0; i < buffer_size; i++) {
            float sample = fast_sin(sound->phase) * sound->amplitude;
            sound->phase += phase_increment;
            
            if (sound->phase >= 6.28318530718) {
                sound->phase -= 6.28318530718;
            }
            
            float envelope = 1.0f;
            
            if (sound->fade_state == 0) {
                envelope = (float)sound->fade_counter / sound->fade_duration;
                sound->fade_counter++;
                if (sound->fade_counter >= sound->fade_duration) {
                    sound->fade_state = 1;
                }
            } else if (sound->fade_state == 2) {
                envelope = 1.0f - ((float)sound->fade_counter / sound->fade_duration);
                sound->fade_counter++;
                if (sound->fade_counter >= sound->fade_duration) {
                    sound->active = false;
                    break;
                }
            } else if (sound->fade_state == 3) {
                sound->timer_counter++;
                if (sound->timer_counter >= sound->timer_samples) {
                    sound->fade_state = 2;
                    sound->fade_counter = 0;
                    sound->fade_duration = FADE_SAMPLES;
                }
            }
            
            float final_sample = sample * envelope * AMPLITUDE;
            
            if (sound->reverb_amount > 0.0f) {
                short delayed_sample = sound->reverb_buffer[sound->reverb_index];
                final_sample += delayed_sample * sound->reverb_amount;
                sound->reverb_buffer[sound->reverb_index] = (short)(final_sample * sound->reverb_decay);
                sound->reverb_index = (sound->reverb_index + 1) % 8820;
                
                float gain_compensation = 1.0f / (1.0f + sound->reverb_amount * 0.5f);
                final_sample *= gain_compensation;
            }
            
            short left_sample = (short)(final_sample * sound->left_amp);
            short right_sample = (short)(final_sample * sound->right_amp);
            
            int left_mixed = buffer[i * 2] + left_sample;
            int right_mixed = buffer[i * 2 + 1] + right_sample;
            
            buffer[i * 2] = (short)((left_mixed > 32767) ? 32767 : (left_mixed < -32768) ? -32768 : left_mixed);
            buffer[i * 2 + 1] = (short)((right_mixed > 32767) ? 32767 : (right_mixed < -32768) ? -32768 : right_mixed);
        }
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    for (int v = 0; v < active_count; v++) {
        int original_index = active_sounds[v].sound_index;
        g_audioSystem.sounds[original_index] = active_sounds[v];
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    
    WavSound active_wav_sounds[MAX_WAV_SOUNDS];
    int active_wav_count = 0;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    for (int v = 0; v < MAX_WAV_SOUNDS; v++) {
        if (g_audioSystem.wav_sounds[v].active) {
            active_wav_sounds[active_wav_count] = g_audioSystem.wav_sounds[v];
            active_wav_sounds[active_wav_count].sound_index = v;
            active_wav_count++;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    
    for (int v = 0; v < active_wav_count; v++) {
        WavSound* wav_sound = &active_wav_sounds[v];
        WavData* wav_data = wav_sound->wav_data;
        
        if (!wav_data || !wav_data->loaded || !wav_data->data) {
            wav_sound->active = false;
            continue;
        }
        
        if (wav_sound->fade_state == 4) {
            wav_sound->delay_counter += buffer_size;
            if (wav_sound->delay_counter >= wav_sound->delay_samples) {
                if (wav_sound->is_timed_after_delay) {
                    wav_sound->fade_state = 3;
                    wav_sound->timer_samples = (int)(wav_sound->delayed_duration_seconds * SAMPLE_RATE);
                    wav_sound->timer_counter = 0;
                } else {
                    wav_sound->fade_state = 0;
                }
                wav_sound->fade_counter = 0;
                wav_sound->fade_duration = FADE_SAMPLES;
            } else {
                continue;
            }
        }
        
        for (int i = 0; i < buffer_size; i++) {
            if (wav_sound->current_position >= wav_data->sample_count) {
                if (wav_sound->repeat) {
                    wav_sound->current_position = 0;
                } else {
                    wav_sound->fade_state = 2;
                    wav_sound->fade_counter = 0;
                    wav_sound->fade_duration = FADE_SAMPLES;
                    break;
                }
            }
            
            float wav_sample;
            if (wav_data->channels == 1) {
                wav_sample = (float)wav_data->data[wav_sound->current_position];
            } else {
                int left_idx = wav_sound->current_position * 2;
                int right_idx = left_idx + 1;
                if (right_idx < wav_data->sample_count * 2) {
                    wav_sample = ((float)wav_data->data[left_idx] + (float)wav_data->data[right_idx]) * 0.5f;
                } else {
                    wav_sample = (float)wav_data->data[left_idx];
                }
            }
            
            wav_sample *= wav_sound->amplitude;
            
            if (wav_sound->reverb_amount > 0.0f) {
                short delayed_sample = wav_sound->reverb_buffer[wav_sound->reverb_index];
                wav_sample += delayed_sample * wav_sound->reverb_amount;
                wav_sound->reverb_buffer[wav_sound->reverb_index] = (short)(wav_sample * wav_sound->reverb_decay);
                wav_sound->reverb_index = (wav_sound->reverb_index + 1) % 8820;
                
                float gain_compensation = 1.0f / (1.0f + wav_sound->reverb_amount * 0.5f);
                wav_sample *= gain_compensation;
            }
            
            float rate_ratio = (float)wav_data->sample_rate / (float)SAMPLE_RATE;
            
            wav_sound->fractional_position += rate_ratio;
            
            int advance_samples = (int)wav_sound->fractional_position;
            if (advance_samples > 0) {
                wav_sound->current_position += advance_samples;
                wav_sound->fractional_position -= advance_samples;
            }
            
            float envelope = 1.0f;
            
            if (wav_sound->fade_state == 0) {
                envelope = (float)wav_sound->fade_counter / (float)wav_sound->fade_duration;
                if (envelope >= 1.0f) {
                    envelope = 1.0f;
                    wav_sound->fade_state = 1;
                }
                wav_sound->fade_counter++;
            } else if (wav_sound->fade_state == 2) {
                envelope = 1.0f - (float)wav_sound->fade_counter / (float)wav_sound->fade_duration;
                if (envelope <= 0.0f) {
                    envelope = 0.0f;
                    wav_sound->active = false;
                    break;
                }
                wav_sound->fade_counter++;
            } else if (wav_sound->fade_state == 3) {
                wav_sound->timer_counter++;
                if (wav_sound->timer_counter >= wav_sound->timer_samples) {
                    wav_sound->fade_state = 2;
                    wav_sound->fade_counter = 0;
                    wav_sound->fade_duration = FADE_SAMPLES;
                }
                envelope = 1.0f;
            }
            
            calculate_stereo_amplitudes(wav_sound->angle, &wav_sound->left_amp, &wav_sound->right_amp);
            
            float final_wav_sample = wav_sample * envelope;
            short left_sample = (short)(final_wav_sample * wav_sound->left_amp);
            short right_sample = (short)(final_wav_sample * wav_sound->right_amp);
            
            int left_mixed = buffer[i * 2] + left_sample;
            int right_mixed = buffer[i * 2 + 1] + right_sample;
            
            buffer[i * 2] = (short)((left_mixed > 32767) ? 32767 : (left_mixed < -32768) ? -32768 : left_mixed);
            buffer[i * 2 + 1] = (short)((right_mixed > 32767) ? 32767 : (right_mixed < -32768) ? -32768 : right_mixed);
        }
    }
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    for (int v = 0; v < active_wav_count; v++) {
        int original_index = active_wav_sounds[v].sound_index;
        g_audioSystem.wav_sounds[original_index] = active_wav_sounds[v];
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}

int audio_play_tone(double frequency, float gain) {
    if (!g_audioSystem.initialized) {
        return -1;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audioSystem.sounds[i].active) {
            Sound* sound = &g_audioSystem.sounds[i];
            
            init_sound_common(sound, frequency, gain, 0.0);
            sound->fade_state = 0;
            
            LeaveCriticalSection(&g_audioSystem.soundLock);
            return i;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    return -1;
}

void audio_stop_sound(int sound_id) {
    if (sound_id < 0 || sound_id >= MAX_SOUNDS || !g_audioSystem.initialized) {
        return;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    Sound* sound = &g_audioSystem.sounds[sound_id];
    if (sound->active && sound->fade_state != 2) {
        sound->fade_state = 2;
        sound->fade_counter = 0;
        sound->fade_duration = FADE_SAMPLES;
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
}

void audio_stop_all_sounds(void) {
    if (!g_audioSystem.initialized) return;
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        Sound* sound = &g_audioSystem.sounds[i];
        if (sound->active && sound->fade_state != 2) {
            sound->fade_state = 2;
            sound->fade_counter = 0;
            sound->fade_duration = FADE_SAMPLES;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
}

int sound_timer(int id, double frequency, float amplitude, double phase, double duration_seconds) {
    if (!g_audioSystem.initialized) {
        return -1;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    if (id >= 0 && id < MAX_SOUNDS) {
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->active = false;
        }
        
        init_sound_common(sound, frequency, amplitude, phase);
        sound->fade_state = 3;
        sound->timer_samples = (int)(duration_seconds * SAMPLE_RATE);
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        return id;
    }
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audioSystem.sounds[i].active) {
            Sound* sound = &g_audioSystem.sounds[i];
            
            init_sound_common(sound, frequency, amplitude, phase);
            sound->fade_state = 3;
            sound->timer_samples = (int)(duration_seconds * SAMPLE_RATE);
            
            LeaveCriticalSection(&g_audioSystem.soundLock);
            return i;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    return -1;
}

int sound_static(int id, double frequency, float amplitude, double phase) {
    if (!g_audioSystem.initialized) {
        return -1;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    if (id >= 0 && id < MAX_SOUNDS) {
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->active = false;
        }
        
        init_sound_common(sound, frequency, amplitude, phase);
        sound->fade_state = 0;
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        return id;
    }
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audioSystem.sounds[i].active) {
            Sound* sound = &g_audioSystem.sounds[i];
            
            init_sound_common(sound, frequency, amplitude, phase);
            sound->fade_state = 0;
            
            LeaveCriticalSection(&g_audioSystem.soundLock);
            return i;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    return -1;
}

void sound_kill(int id) {
    audio_stop_sound(id);
}

void sound_kill_all(void) {
    audio_stop_all_sounds();
}

int sound_starter_timer(int id, double frequency, float amplitude, double phase, double duration_seconds, double start_delay_seconds) {
    if (!g_audioSystem.initialized) {
        return -1;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    if (id >= 0 && id < MAX_SOUNDS) {
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->active = false;
        }
        
        init_sound_common(sound, frequency, amplitude, phase);
        sound->fade_state = 4;
        sound->delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
        sound->is_timed_after_delay = true;
        sound->delayed_duration_seconds = duration_seconds;
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        return id;
    }
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audioSystem.sounds[i].active) {
            Sound* sound = &g_audioSystem.sounds[i];
            
            init_sound_common(sound, frequency, amplitude, phase);
            sound->fade_state = 4;
            sound->delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
            sound->is_timed_after_delay = true;
            sound->delayed_duration_seconds = duration_seconds;
            
            LeaveCriticalSection(&g_audioSystem.soundLock);
            return i;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    return -1;
}

int sound_starter_static(int id, double frequency, float amplitude, double phase, double start_delay_seconds) {
    if (!g_audioSystem.initialized) {
        return -1;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    
    if (id >= 0 && id < MAX_SOUNDS) {
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->active = false;
        }
        
        init_sound_common(sound, frequency, amplitude, phase);
        sound->fade_state = 4;
        sound->delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
        sound->is_timed_after_delay = false;
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        return id;
    }
    
    for (int i = 0; i < MAX_SOUNDS; i++) {
        if (!g_audioSystem.sounds[i].active) {
            Sound* sound = &g_audioSystem.sounds[i];
            
            init_sound_common(sound, frequency, amplitude, phase);
            sound->fade_state = 4;
            sound->delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
            sound->is_timed_after_delay = false;
            
            LeaveCriticalSection(&g_audioSystem.soundLock);
            return i;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.soundLock);
    return -1;
}

void sound_angle(int id, float angle) {
    if (!g_audioSystem.initialized) {
        return;
    }
    
    if (id >= 0 && id < MAX_SOUNDS) {
        EnterCriticalSection(&g_audioSystem.soundLock);
        
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->angle = angle;
            calculate_stereo_amplitudes(angle, &sound->left_amp, &sound->right_amp);
        }
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        
    } else if (id >= 100 && id < 100 + MAX_WAV_SOUNDS) {
        int wav_id = id - 100;
        
        EnterCriticalSection(&g_audioSystem.wavLock);
        
        WavSound* wav_sound = &g_audioSystem.wav_sounds[wav_id];
        if (wav_sound->active) {
            wav_sound->angle = angle;
            calculate_stereo_amplitudes(angle, &wav_sound->left_amp, &wav_sound->right_amp);
        }
        
        LeaveCriticalSection(&g_audioSystem.wavLock);
    }
}

void sound_reverb(int id, float amount, float decay) {
    if (!g_audioSystem.initialized) {
        return;
    }
    
    if (amount < 0.0f) amount = 0.0f;
    if (amount > 1.0f) amount = 1.0f;
    if (decay < 0.1f) decay = 0.1f;
    if (decay > 0.9f) decay = 0.9f;
    
    if (id >= 0 && id < MAX_SOUNDS) {
        EnterCriticalSection(&g_audioSystem.soundLock);
        
        Sound* sound = &g_audioSystem.sounds[id];
        if (sound->active) {
            sound->reverb_amount = amount;
            sound->reverb_decay = decay;
        }
        
        LeaveCriticalSection(&g_audioSystem.soundLock);
        
    } else if (id >= 100 && id < 100 + MAX_WAV_SOUNDS) {
        int wav_id = id - 100;
        
        EnterCriticalSection(&g_audioSystem.wavLock);
        
        WavSound* wav_sound = &g_audioSystem.wav_sounds[wav_id];
        if (wav_sound->active) {
            wav_sound->reverb_amount = amount;
            wav_sound->reverb_decay = decay;
        }
        
        LeaveCriticalSection(&g_audioSystem.wavLock);
    }
}




















#include <immintrin.h>
#include <intrin.h>

#pragma pack(push, 1)
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_size;
    uint16_t format;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} WavHeader;
#pragma pack(pop)

static inline void convert_8bit_to_16bit_simd(const unsigned char* src, short* dst, int sample_count) {
    int i = 0;
    
    if (sample_count >= 16) {
        const __m128i offset = _mm_set1_epi8(128);
        const __m128i zero = _mm_setzero_si128();
        
        for (; i <= sample_count - 16; i += 16) {
            __m128i samples_8bit = _mm_loadu_si128((const __m128i*)(src + i));
            
            samples_8bit = _mm_sub_epi8(samples_8bit, offset);
            
            __m128i samples_16bit_lo = _mm_unpacklo_epi8(samples_8bit, samples_8bit);
            __m128i samples_16bit_hi = _mm_unpackhi_epi8(samples_8bit, samples_8bit);
            
            samples_16bit_lo = _mm_srai_epi16(samples_16bit_lo, 8);
            samples_16bit_hi = _mm_srai_epi16(samples_16bit_hi, 8);
            samples_16bit_lo = _mm_slli_epi16(samples_16bit_lo, 8);
            samples_16bit_hi = _mm_slli_epi16(samples_16bit_hi, 8);
            
            _mm_storeu_si128((__m128i*)(dst + i), samples_16bit_lo);
            _mm_storeu_si128((__m128i*)(dst + i + 8), samples_16bit_hi);
        }
    }
    
    for (; i < sample_count; i++) {
        dst[i] = (short)((src[i] - 128) * 256);
    }
}

static inline void convert_24bit_to_16bit_simd(const unsigned char* src, short* dst, int sample_count) {
    int i = 0;
    
    for (; i <= sample_count - 4; i += 4) {
        __m128i bytes = _mm_loadu_si128((const __m128i*)(src + i * 3));
        
        int sample0 = (src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16));
        int sample1 = (src[i*3+3] | (src[i*3+4] << 8) | (src[i*3+5] << 16));
        int sample2 = (src[i*3+6] | (src[i*3+7] << 8) | (src[i*3+8] << 16));
        int sample3 = (src[i*3+9] | (src[i*3+10] << 8) | (src[i*3+11] << 16));
        
        if (sample0 & 0x800000) sample0 |= 0xFF000000;
        if (sample1 & 0x800000) sample1 |= 0xFF000000;
        if (sample2 & 0x800000) sample2 |= 0xFF000000;
        if (sample3 & 0x800000) sample3 |= 0xFF000000;
        
        __m128i samples_32bit = _mm_set_epi32(sample3, sample2, sample1, sample0);
        __m128i samples_16bit = _mm_packs_epi32(_mm_srai_epi32(samples_32bit, 8), _mm_setzero_si128());
        
        _mm_storel_epi64((__m128i*)(dst + i), samples_16bit);
    }
    
    for (; i < sample_count; i++) {
        int sample24 = (src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16));
        if (sample24 & 0x800000) {
            sample24 |= 0xFF000000;
        }
        dst[i] = (short)(sample24 >> 8);
    }
}

static inline void convert_32bit_to_16bit_simd(const int* src, short* dst, int sample_count) {
    int i = 0;
    
    for (; i <= sample_count - 8; i += 8) {
        __m128i samples_32bit_0 = _mm_loadu_si128((const __m128i*)(src + i));
        __m128i samples_32bit_1 = _mm_loadu_si128((const __m128i*)(src + i + 4));
        
        samples_32bit_0 = _mm_srai_epi32(samples_32bit_0, 16);
        samples_32bit_1 = _mm_srai_epi32(samples_32bit_1, 16);
        
        __m128i samples_16bit = _mm_packs_epi32(samples_32bit_0, samples_32bit_1);
        
        _mm_storeu_si128((__m128i*)(dst + i), samples_16bit);
    }
    
    for (; i < sample_count; i++) {
        dst[i] = (short)(src[i] >> 16);
    }
}

static bool has_sse2_support(void) {
    static int sse2_checked = -1;
    
    if (sse2_checked == -1) {
        int cpuinfo[4];
        __cpuid(cpuinfo, 1);
        sse2_checked = (cpuinfo[3] & (1 << 26)) ? 1 : 0;
    }
    
    return sse2_checked == 1;
}

static void init_wav_sound_common(WavSound* sound, WavData* wav_data, float amplitude) {
    sound->wav_data = wav_data;
    sound->current_position = 0;
    sound->fractional_position = 0.0f;
    sound->amplitude = amplitude;
    sound->angle = 0.0f;
    sound->left_amp = 1.0f;
    sound->right_amp = 1.0f;
    sound->active = true;
    sound->fade_counter = 0;
    sound->fade_duration = FADE_SAMPLES;
    sound->timer_samples = 0;
    sound->timer_counter = 0;
    sound->delay_samples = 0;
    sound->delay_counter = 0;
    sound->is_timed_after_delay = false;
    sound->delayed_duration_seconds = 0.0;
    sound->reverb_amount = 0.0f;
    sound->reverb_decay = 0.5f;
    sound->reverb_index = 0;
    memset(sound->reverb_buffer, 0, sizeof(sound->reverb_buffer));
}

bool load_wav_file(const char* filename) {
    if (!g_audioSystem.initialized) {
        return false;
    }

    EnterCriticalSection(&g_audioSystem.wavLock);
    
    // Check if already loaded
    for (int i = 0; i < g_audioSystem.wav_cache_count; i++) {
        if (strcmp(g_audioSystem.wav_cache[i].filename, filename) == 0 && 
            g_audioSystem.wav_cache[i].loaded) {
            LeaveCriticalSection(&g_audioSystem.wavLock);
            return true;
        }
    }
    
    if (g_audioSystem.wav_cache_count >= 32) {
        printf("WAV cache full! Cannot load more files.\n");
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "source/sound/%s", filename);
    
    FILE* file = fopen(full_path, "rb");
    if (!file) {
        printf("Failed to open WAV file: %s\n", full_path);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, file) != 1) {
        printf("Failed to read WAV header: %s\n", filename);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    // Validate WAV format
    if (strncmp(header.riff, "RIFF", 4) != 0 || 
        strncmp(header.wave, "WAVE", 4) != 0 ||
        strncmp(header.fmt, "fmt ", 4) != 0 ||
        strncmp(header.data, "data", 4) != 0) {
        printf("Invalid WAV format: %s\n", filename);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    if (header.format != 1) {
        printf("Unsupported WAV format (not PCM): %s\n", filename);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    if (header.bits_per_sample != 16 && header.bits_per_sample != 8 && header.bits_per_sample != 24 && header.bits_per_sample != 32) {
        printf("Unsupported bit depth (%d-bit): %s (supported: 8, 16, 24, 32-bit)\n", header.bits_per_sample, filename);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    int bytes_per_sample = header.bits_per_sample / 8;
    int sample_count = header.data_size / (header.channels * bytes_per_sample);
    
    unsigned char* raw_data = (unsigned char*)malloc(header.data_size);
    if (!raw_data) {
        printf("Failed to allocate memory for raw WAV data: %s\n", filename);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    // Read raw audio data
    if (fread(raw_data, header.data_size, 1, file) != 1) {
        printf("Failed to read WAV audio data: %s\n", filename);
        free(raw_data);
        fclose(file);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    fclose(file);
    
    // Allocate memory for converted 16-bit audio data
    short* audio_data = (short*)malloc(sample_count * header.channels * sizeof(short));
    if (!audio_data) {
        printf("Failed to allocate memory for converted WAV data: %s\n", filename);
        free(raw_data);
        LeaveCriticalSection(&g_audioSystem.wavLock);
        return false;
    }
    
    // Convert to 16-bit based on original bit depth
    bool use_simd = has_sse2_support();
    
    if (header.bits_per_sample == 8) {
        // 8-bit to 16-bit conversion (unsigned to signed)
        if (use_simd && sample_count * header.channels >= 16) {
            convert_8bit_to_16bit_simd(raw_data, audio_data, sample_count * header.channels);
        } else {
            // Fallback to scalar conversion
            unsigned char* src = raw_data;
            for (int i = 0; i < sample_count * header.channels; i++) {
                audio_data[i] = (short)((src[i] - 128) * 256);
            }
        }
    } else if (header.bits_per_sample == 16) {
        // Already 16-bit, just copy
        memcpy(audio_data, raw_data, sample_count * header.channels * sizeof(short));
    } else if (header.bits_per_sample == 24) {
        // 24-bit to 16-bit conversion
        if (use_simd && sample_count * header.channels >= 4) {
            convert_24bit_to_16bit_simd(raw_data, audio_data, sample_count * header.channels);
        } else {
            // Fallback to scalar conversion
            unsigned char* src = raw_data;
            for (int i = 0; i < sample_count * header.channels; i++) {
                int sample24 = (src[i*3] | (src[i*3+1] << 8) | (src[i*3+2] << 16));
                if (sample24 & 0x800000) {
                    sample24 |= 0xFF000000;
                }
                audio_data[i] = (short)(sample24 >> 8);
            }
        }
    } else if (header.bits_per_sample == 32) {
        // 32-bit to 16-bit conversion
        if (use_simd && sample_count * header.channels >= 8) {
            convert_32bit_to_16bit_simd((const int*)raw_data, audio_data, sample_count * header.channels);
        } else {
            // Fallback to scalar conversion
            int* src32 = (int*)raw_data;
            for (int i = 0; i < sample_count * header.channels; i++) {
                audio_data[i] = (short)(src32[i] >> 16);
            }
        }
    }
    
    free(raw_data);
    
    // Store in cache
    WavData* wav_data = &g_audioSystem.wav_cache[g_audioSystem.wav_cache_count];
    wav_data->data = audio_data;
    wav_data->sample_count = sample_count;
    wav_data->channels = header.channels;
    wav_data->sample_rate = header.sample_rate;
    strncpy(wav_data->filename, filename, sizeof(wav_data->filename) - 1);
    wav_data->filename[sizeof(wav_data->filename) - 1] = '\0';
    wav_data->loaded = true;
    
    g_audioSystem.wav_cache_count++;
    
    printf("Loaded WAV file: %s (%d samples, %d channels, %d Hz, %d-bit)%s\n", 
           filename, sample_count, header.channels, header.sample_rate, header.bits_per_sample,
           use_simd ? " [SIMD]" : " [Scalar]");
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    return true;
}

void unload_wav_file(const char* filename) {
    if (!g_audioSystem.initialized) {
        return;
    }

    EnterCriticalSection(&g_audioSystem.wavLock);
    
    for (int i = 0; i < g_audioSystem.wav_cache_count; i++) {
        if (strcmp(g_audioSystem.wav_cache[i].filename, filename) == 0 && 
            g_audioSystem.wav_cache[i].loaded) {
            
            // Free audio data
            if (g_audioSystem.wav_cache[i].data) {
                free(g_audioSystem.wav_cache[i].data);
                g_audioSystem.wav_cache[i].data = NULL;
            }
            
            g_audioSystem.wav_cache[i].loaded = false;
            
            // Shift cache entries down
            for (int j = i; j < g_audioSystem.wav_cache_count - 1; j++) {
                g_audioSystem.wav_cache[j] = g_audioSystem.wav_cache[j + 1];
            }
            g_audioSystem.wav_cache_count--;
            
            printf("Unloaded WAV file: %s\n", filename);
            break;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}

void unload_all_wav_files(void) {
    if (!g_audioSystem.initialized) {
        return;
    }

    EnterCriticalSection(&g_audioSystem.wavLock);
    
    for (int i = 0; i < g_audioSystem.wav_cache_count; i++) {
        if (g_audioSystem.wav_cache[i].loaded && g_audioSystem.wav_cache[i].data) {
            free(g_audioSystem.wav_cache[i].data);
            g_audioSystem.wav_cache[i].data = NULL;
        }
        g_audioSystem.wav_cache[i].loaded = false;
    }
    
    g_audioSystem.wav_cache_count = 0;
    printf("Unloaded all WAV files from cache\n");
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}

// Find WAV data by filename
static WavData* find_wav_data(const char* filename) {
    for (int i = 0; i < g_audioSystem.wav_cache_count; i++) {
        if (strcmp(g_audioSystem.wav_cache[i].filename, filename) == 0 && 
            g_audioSystem.wav_cache[i].loaded) {
            return &g_audioSystem.wav_cache[i];
        }
    }
    return NULL;
}

static int get_or_create_wav_sound(int id, WavData* wav_data, float amplitude) {
    if (id >= 0 && id < MAX_WAV_SOUNDS) {
        WavSound* sound = &g_audioSystem.wav_sounds[id];
        if (sound->active) {
            sound->fade_state = 2;
            sound->fade_counter = 0;
            sound->fade_duration = FADE_SAMPLES;
        }
        init_wav_sound_common(sound, wav_data, amplitude);
        return id;
    }
    
    for (int i = 0; i < MAX_WAV_SOUNDS; i++) {
        if (!g_audioSystem.wav_sounds[i].active) {
            init_wav_sound_common(&g_audioSystem.wav_sounds[i], wav_data, amplitude);
            return i;
        }
    }
    return -1;
}

int sound_wav_timer(int id, const char* filename, float amplitude, double duration_seconds) {
    if (!g_audioSystem.initialized || !load_wav_file(filename)) return -1;
    
    WavData* wav_data = find_wav_data(filename);
    if (!wav_data) return -1;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    int sound_id = get_or_create_wav_sound(id, wav_data, amplitude);
    if (sound_id >= 0) {
        g_audioSystem.wav_sounds[sound_id].fade_state = 3;
        g_audioSystem.wav_sounds[sound_id].timer_samples = (int)(duration_seconds * SAMPLE_RATE);
        g_audioSystem.wav_sounds[sound_id].repeat = false;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    return sound_id;
}

int sound_wav_repeat(int id, const char* filename, float amplitude) {
    if (!g_audioSystem.initialized || !load_wav_file(filename)) return -1;
    
    WavData* wav_data = find_wav_data(filename);
    if (!wav_data) return -1;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    int sound_id = get_or_create_wav_sound(id, wav_data, amplitude);
    if (sound_id >= 0) {
        g_audioSystem.wav_sounds[sound_id].fade_state = 0;
        g_audioSystem.wav_sounds[sound_id].repeat = true;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    return sound_id;
}

int sound_wav_starter_timer(int id, const char* filename, float amplitude, double duration_seconds, double start_delay_seconds) {
    if (!g_audioSystem.initialized || !load_wav_file(filename)) return -1;
    
    WavData* wav_data = find_wav_data(filename);
    if (!wav_data) return -1;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    int sound_id = get_or_create_wav_sound(id, wav_data, amplitude);
    if (sound_id >= 0) {
        g_audioSystem.wav_sounds[sound_id].fade_state = 4;
        g_audioSystem.wav_sounds[sound_id].delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
        g_audioSystem.wav_sounds[sound_id].is_timed_after_delay = true;
        g_audioSystem.wav_sounds[sound_id].delayed_duration_seconds = duration_seconds;
        g_audioSystem.wav_sounds[sound_id].repeat = false;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    return sound_id;
}

int sound_wav_starter_repeat(int id, const char* filename, float amplitude, double start_delay_seconds) {
    if (!g_audioSystem.initialized || !load_wav_file(filename)) return -1;
    
    WavData* wav_data = find_wav_data(filename);
    if (!wav_data) return -1;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    int sound_id = get_or_create_wav_sound(id, wav_data, amplitude);
    if (sound_id >= 0) {
        g_audioSystem.wav_sounds[sound_id].fade_state = 4;
        g_audioSystem.wav_sounds[sound_id].delay_samples = (int)(start_delay_seconds * SAMPLE_RATE);
        g_audioSystem.wav_sounds[sound_id].is_timed_after_delay = false;
        g_audioSystem.wav_sounds[sound_id].repeat = true;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
    return sound_id;
}

void sound_wav_kill(int id) {
    if (id < 0 || id >= MAX_WAV_SOUNDS || !g_audioSystem.initialized) {
        return;
    }
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    WavSound* sound = &g_audioSystem.wav_sounds[id];
    if (sound->active && sound->fade_state != 2) {
        sound->fade_state = 2; // Start fade out
        sound->fade_counter = 0;
        sound->fade_duration = FADE_SAMPLES;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}

void sound_wav_kill_all(void) {
    if (!g_audioSystem.initialized) {
        return;
    }
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    for (int i = 0; i < MAX_WAV_SOUNDS; i++) {
        WavSound* sound = &g_audioSystem.wav_sounds[i];
        if (sound->active && sound->fade_state != 2) {
            sound->fade_state = 2; // Start fade out
            sound->fade_counter = 0;
            sound->fade_duration = FADE_SAMPLES;
        }
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}

void sound_wav_set_amplitude(int id, float amplitude) {
    if (id < 0 || id >= MAX_WAV_SOUNDS || !g_audioSystem.initialized) {
        return;
    }
    
    // Clamp amplitude to valid range
    if (amplitude < 0.0f) amplitude = 0.0f;
    if (amplitude > 1.0f) amplitude = 1.0f;
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    
    WavSound* sound = &g_audioSystem.wav_sounds[id];
    if (sound->active) {
        sound->amplitude = amplitude;
    }
    
    LeaveCriticalSection(&g_audioSystem.wavLock);
}




















////////////////////// SoundManager Class Implementation //////////////////////

////////////////////// Initialize audio system
bool SoundManager::AudioInit(void) {
    return audio_init();
}

////////////////////// Shutdown audio system
void SoundManager::AudioShutdown(void) {
    audio_shutdown();
}

////////////////////// Create a timed sound with specified parameters
int SoundManager::SoundTimer(int id, double frequency, float amplitude, double phase, double duration_seconds) {
    return sound_timer(id, frequency, amplitude, phase, duration_seconds);
}

////////////////////// Create a static (continuous) sound with specified parameters
int SoundManager::SoundStatic(int id, double frequency, float amplitude, double phase) {
    return sound_static(id, frequency, amplitude, phase);
}

////////////////////// Create a timed sound with a start delay
int SoundManager::SoundStarterTimer(int id, double frequency, float amplitude, double phase, double duration_seconds, double start_delay_seconds) {
    return sound_starter_timer(id, frequency, amplitude, phase, duration_seconds, start_delay_seconds);
}

////////////////////// Create a static sound with a start delay
int SoundManager::SoundStarterStatic(int id, double frequency, float amplitude, double phase, double start_delay_seconds) {
    return sound_starter_static(id, frequency, amplitude, phase, start_delay_seconds);
}

////////////////////// Stop a specific sound by ID
void SoundManager::SoundKill(int id) {
    sound_kill(id);
}

////////////////////// Stop all active sounds
void SoundManager::SoundKillAll(void) {
    sound_kill_all();
}

////////////////////// Set the stereo angle for a sound
void SoundManager::SoundAngle(int id, float angle) {
    sound_angle(id, angle);
}

////////////////////// Set reverb effect for a sound
void SoundManager::SoundReverb(int id, float amount, float decay) {
    sound_reverb(id, amount, decay);
}

////////////////////// Check if a sound is currently playing
bool SoundManager::SoundIsPlaying(int id) {
    if (id < 0 || id >= MAX_SOUNDS) {
        return false;
    }
    
    EnterCriticalSection(&g_audioSystem.soundLock);
    bool is_playing = g_audioSystem.sounds[id].active;
    LeaveCriticalSection(&g_audioSystem.soundLock);
    
    return is_playing;
}

////////////////////// Play a simple tone
int SoundManager::AudioPlayTone(double frequency, float gain) {
    return audio_play_tone(frequency, gain);
}

////////////////////// Stop a specific audio sound
void SoundManager::AudioStopSound(int sound_id) {
    audio_stop_sound(sound_id);
}

////////////////////// Stop all audio sounds
void SoundManager::AudioStopAllSounds(void) {
    audio_stop_all_sounds();
}

////////////////////// Play a WAV file for a specified duration
int SoundManager::SoundWavTimer(int id, const char* filename, float amplitude, double duration_seconds) {
    return sound_wav_timer(id, filename, amplitude, duration_seconds);
}

////////////////////// Play a WAV file on repeat
int SoundManager::SoundWavRepeat(int id, const char* filename, float amplitude) {
    return sound_wav_repeat(id, filename, amplitude);
}

////////////////////// Play a WAV file for a specified duration with a start delay
int SoundManager::SoundWavStarterTimer(int id, const char* filename, float amplitude, double duration_seconds, double start_delay_seconds) {
    return sound_wav_starter_timer(id, filename, amplitude, duration_seconds, start_delay_seconds);
}

////////////////////// Play a WAV file on repeat with a start delay
int SoundManager::SoundWavStarterRepeat(int id, const char* filename, float amplitude, double start_delay_seconds) {
    return sound_wav_starter_repeat(id, filename, amplitude, start_delay_seconds);
}

////////////////////// Stop a specific WAV sound by ID
void SoundManager::SoundWavKill(int id) {
    sound_wav_kill(id);
}

////////////////////// Stop all active WAV sounds
void SoundManager::SoundWavKillAll(void) {
    sound_wav_kill_all();
}

////////////////////// Set the amplitude of a WAV sound
void SoundManager::SoundWavSetAmplitude(int id, float amplitude) {
    sound_wav_set_amplitude(id, amplitude);
}

////////////////////// Check if a WAV sound is currently playing
bool SoundManager::SoundWavIsPlaying(int id) {
    if (id < 0 || id >= MAX_WAV_SOUNDS) {
        return false;
    }
    
    EnterCriticalSection(&g_audioSystem.wavLock);
    bool is_playing = g_audioSystem.wav_sounds[id].active;
    LeaveCriticalSection(&g_audioSystem.wavLock);
    
    return is_playing;
}

////////////////////// Load a WAV file into memory
bool SoundManager::LoadWavFile(const char* filename) {
    return load_wav_file(filename);
}

////////////////////// Unload a specific WAV file from memory
void SoundManager::UnloadWavFile(const char* filename) {
    unload_wav_file(filename);
}

////////////////////// Unload all WAV files from memory
void SoundManager::UnloadAllWavFiles(void) {
    unload_all_wav_files();
}
