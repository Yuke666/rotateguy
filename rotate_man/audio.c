#include <SDL2/SDL_audio.h>
#include <string.h>
#include "audio.h"

#define MIN(x,y) ((x)<(y)?(x):(y))

#define SAMPLERATE 44100
#define	FRACBITS 28
#define SAMPLING_SHIFT 2

static void AudioThread(void *data, Uint8 *ustream, int len){

	AudioThread_t *at = (AudioThread_t *)data;

	s8 *stream = (s8 *)ustream;

	memset(stream, 0, len);

	if(at->modPlayer.file)
		MODPlayer_MixAudio(&at->modPlayer, stream, len);

	memcpy(at->samples, stream, len);
	at->nSamples = len;

	if(at->effect.curr >> FRACBITS < at->effect.length){
	
		u32 k, i;

		s8 curr; 
		
		s32 maxVolumeSq = AUDIO_MAX_VOLUME*AUDIO_MAX_VOLUME;
		s32 volume = at->soundsVolume * at->effect.volume;
		// max = MAX_AUDIO_VOLUME * MAX_AUDIO_VOLUME = maxVolumeSq
		s32 maxMinuxVolume = AUDIO_MAX_VOLUME - volume;

		for(k = 0; k < (u32)len; k++){

			at->effect.curr += at->effect.increment;

			i = at->effect.curr >> FRACBITS;

			if(i >= at->effect.length)
				break;

			curr = (((s32)(at->effect.samples[i] * volume)) / maxVolumeSq);
			
			curr += ((s32)stream[k]*maxMinuxVolume) / maxVolumeSq;

			stream[k] = curr;
		}
	}

}


void Audio_Play(AudioThread_t *at, const s8 *samples, u16 len, u8 volume, u16 freq){
	SDL_LockAudio();
	at->effect.samples = samples;
	at->effect.curr = 0;
	at->effect.volume = volume;
	at->effect.length = len;
	at->effect.increment = (freq << FRACBITS)/SAMPLERATE;
	at->effect.increment = ((mod_u64)(8363*1712/freq) << (FRACBITS-SAMPLING_SHIFT))/SAMPLERATE;
	SDL_UnlockAudio();
}

void Audio_SetMusicVolume(AudioThread_t *at, u8 vol){
	SDL_LockAudio();
	at->modPlayer.volume = MIN(vol, AUDIO_MAX_VOLUME);
	SDL_UnlockAudio();
}

void Audio_SetSoundsVolume(AudioThread_t *at, u8 vol){
	SDL_LockAudio();
	at->soundsVolume = MIN(vol, AUDIO_MAX_VOLUME);
	SDL_UnlockAudio();
}

void Audio_PlayMOD(AudioThread_t *at, MODFile_t *file){
	SDL_LockAudio();
	MODPlayer_Play(&at->modPlayer, file);
	SDL_UnlockAudio();
}

void Audio_StopMOD(AudioThread_t *at){
	SDL_LockAudio();
	at->modPlayer.file = NULL;
	SDL_UnlockAudio();
}

void Audio_Init(AudioThread_t *at){

	memset(at, 0, sizeof(AudioThread_t));

	at->soundsVolume = 64;

	SDL_AudioSpec fmt;

	fmt.freq = SAMPLERATE;
	fmt.format = AUDIO_S8;
	fmt.channels = 1;
	fmt.samples = AUDIO_MAX_SAMPLES;
	fmt.callback = AudioThread;
	fmt.userdata = at;

	if (SDL_OpenAudio(&fmt, NULL) < 0){
		printf("Unable to open audio: %s\n", SDL_GetError());
		return;
	}

	SDL_PauseAudio(0);
}

void Audio_Close(AudioThread_t *at){
	(void)at;
	SDL_CloseAudio();
}