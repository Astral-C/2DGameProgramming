#ifndef AUDIO_SYS
#define AUDIO_SYS
#include <SDL.h>
#include "gfc_audio.h"
#include "tracker.h"

typedef struct {
    ModTracker* _tracker_playback;
    SDL_AudioSpec device_format;
    SDL_AudioDeviceID device;
} AudioSys;

void init_audio_sys();

void audio_open_mod(char* path);
void audio_play_mod();
void audio_pause_mod();
void audio_close_mod();

void exit_audio_sys();

#endif