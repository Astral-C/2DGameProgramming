#include "audio.h"

static AudioSys sys = {0};

void audio_sys_update(void* userdata, uint8_t* stream, int len){
	if(sys._tracker_playback != NULL){
		len /= sizeof(int16_t);
		tracker_mod_update(sys._tracker_playback, (int16_t*)stream, (uint32_t)len);
	}
}

void init_audio_sys(){
	SDL_AudioSpec target_format;
	target_format.freq = 44100;
	target_format.format = AUDIO_S16;
	target_format.channels = 2;
	target_format.samples = 4096;
	target_format.callback = audio_sys_update;
	target_format.userdata = NULL;

	sys.device = SDL_OpenAudioDevice(NULL, 0, &target_format, &sys.device_format, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
	atexit(exit_audio_sys);
}

void audio_open_mod(char* path){
	if(sys._tracker_playback == NULL){
		sys._tracker_playback = (ModTracker*)malloc(sizeof(ModTracker));
	}

	tracker_open_mod(sys._tracker_playback, path);
	audio_pause_mod();
	tracker_mod_set_sample_rate(sys._tracker_playback, 44100);
}

void audio_play_mod(){
	SDL_PauseAudioDevice(sys.device, 0);
}

void audio_pause_mod(){
	SDL_PauseAudioDevice(sys.device, 1);
}

void audio_close_mod(){
	audio_pause_mod();
	if(sys._tracker_playback != NULL){
		tracker_close_mod(sys._tracker_playback);
		free(sys._tracker_playback);
		sys._tracker_playback = NULL;
	}
}

void exit_audio_sys(){
	audio_close_mod();
	SDL_CloseAudioDevice(sys.device);
}