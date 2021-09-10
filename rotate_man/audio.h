#ifndef AUDIO_DEF
#define AUDIO_DEF

#include "types.h"
#include "mod.h"

#define AUDIO_MAX_VOLUME 64
#define AUDIO_MAX_SAMPLES 1024

typedef struct {
	u8 			volume;
	u16 		length;
	const s8 	*samples;
	u64 		curr;
	u64 		increment;
} SoundEffect_t;

typedef struct {
	u8 				soundsVolume;
	SoundEffect_t 	effect;
	MODPlayer_t 	modPlayer;
	s8 				samples[AUDIO_MAX_SAMPLES];
	u32 			nSamples;
} AudioThread_t;

void Audio_Init(AudioThread_t *at);
void Audio_SetMusicVolume(AudioThread_t *at, u8 vol);
void Audio_SetSoundsVolume(AudioThread_t *at, u8 vol);
void Audio_Close(AudioThread_t *at);
void Audio_Play(AudioThread_t *at, const s8 *samples, u16 len, u8 volume, u16 freq);
void Audio_PlayMOD(AudioThread_t *at, MODFile_t *file);
void Audio_StopMOD(AudioThread_t *at);

#endif