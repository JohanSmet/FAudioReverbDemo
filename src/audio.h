#ifndef FAUDIOFILTERDEMO_AUDIO_H
#define FAUDIOFILTERDEMO_AUDIO_H

#include <stddef.h>
#include <stdint.h>

const float PI = 3.14159265358979323846f;

// types
struct AudioContext;
struct AudioVoice;
struct AudioFilter;

enum AudioEngine {
	AudioEngine_XAudio2,
	AudioEngine_FAudio
};

enum AudioSampleWave {
	AudioWave_SnareDrum01 = 0,
	AudioWave_SnareDrum02,
	AudioWave_SnareDrum03,
};

#pragma pack(push, 1)

struct ReverbI3DL2Parameters
{
	float WetDryMix;
	int Room; 
	int RoomHF;
	float RoomRolloffFactor;
	float DecayTime;
	float DecayHFRatio;
	int Reflections;
	float ReflectionsDelay;
	int Reverb;
	float ReverbDelay;
	float Diffusion;
	float Density;
	float HFReference;
};

struct ReverbParameters
{
	float WetDryMix;
	uint32_t ReflectionsDelay;
	uint8_t ReverbDelay;
	uint8_t RearDelay;
	uint8_t PositionLeft;
	uint8_t PositionRight;
	uint8_t PositionMatrixLeft;
	uint8_t PositionMatrixRight;
	uint8_t EarlyDiffusion;
	uint8_t LateDiffusion;
	uint8_t LowEQGain;
	uint8_t LowEQCutoff;
	uint8_t HighEQGain;
	uint8_t HighEQCutoff;
	float RoomFilterFreq;
	float RoomFilterMain;
	float RoomFilterHF;
	float ReflectionsGain;
	float ReverbGain;
	float DecayTime;
	float Density;
	float RoomSize;
};

struct ReverbTestParameters
{
	float WetDryMix;				/* 0 - 100 (0 = fully dry, 100 = fully wet) */
	uint32_t ReflectionsDelay;		/* 0 - 300 ms */
	uint8_t ReverbDelay;			/* 0 - 85 ms */
	uint8_t RearDelay;				/* 0 - 5 ms NOT USED YET */
	uint8_t PositionLeft;			/* 0 - 30 NOT USED YET */
	uint8_t PositionRight;			/* 0 - 30 NOT USED YET */
	uint8_t PositionMatrixLeft;		/* 0 - 30 NOT USED YET */
	uint8_t PositionMatrixRight;	/* 0 - 30 NOT USED YET */
	uint8_t EarlyDiffusion;			/* 0 - 15 */
	uint8_t LateDiffusion;			/* 0 - 15 */
	uint8_t LowEQGain;				/* 0 - 12 (formula dB = LowEqGain - 8) */
	uint8_t LowEQCutoff;			/* 0 - 9  (formula Hz = 50 + (LowEqCutoff * 50)) */
	uint8_t HighEQGain;				/* 0 - 8  (formula dB = HighEQGain - 8)*/
	uint8_t HighEQCutoff;			/* 0 - 14 (formula Hz = 1000 + (HighEqCutoff * 500))*/
	float RoomFilterFreq;			/* 20 - 20000Hz */
	float RoomFilterMain;			/* -100 - 0dB */
	float RoomFilterHF;				/* -100 - 0dB */
	float ReflectionsGain;			/* -100 - 20dB */
	float ReverbGain;				/* -100 - 20dB */
	float DecayTime;				/* 0.1 - .... ms */
	float Density;					/* 0 - 100 % */
	float RoomSize;					/* 1 - 100 feet */


	/* extra parameters (for testing/tuning) */
	float Comb1Delay;				// 0 - 100 ms (31.71)
	float Comb2Delay;				// 0 - 100 ms (31.71)
	float LPFComb1Delay;			// 0 - 100 ms (31.71)
	float LPFComb1Gain;				// -1 - 1 (0.7)
	float LPFComb2Delay;			// 0 - 100 ms (31.71)
	float LPFComb2Gain;				// -1 - 1 (0.7)

	float InDiffusionLength1;		// 0 - 100 ms (13.28)
	float InDiffusionLength2;		// 0 - 100 ms (28.13)
	float OutDiffusionLength;		// 0 - 100 ms (13.28)
};

#pragma pack(pop)

extern const char *audio_sample_filenames[];
extern const char *audio_reverb_preset_names[];
extern const ReverbI3DL2Parameters audio_reverb_presets_i3dl2[];
extern const ReverbParameters	   *audio_reverb_presets;
extern const size_t				   audio_reverb_preset_count;

typedef void(*PFN_AUDIO_DESTROY_CONTEXT)(AudioContext *p_context);

typedef AudioVoice *(*PFN_AUDIO_CREATE_VOICE)(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels);
typedef void (*PFN_AUDIO_VOICE_DESTROY)(AudioVoice *p_voice);
typedef void (*PFN_AUDIO_VOICE_SET_VOLUME)(AudioVoice *p_voice, float p_volume);
typedef void (*PFN_AUDIO_VOICE_SET_FREQUENCY)(AudioVoice *p_vioce, float p_frequency);

typedef void (*PFN_AUDIO_WAVE_LOAD)(AudioContext *p_context, AudioSampleWave sample);
typedef void (*PFN_AUDIO_WAVE_PLAY)(AudioContext *p_context);

typedef void(*PFN_AUDIO_EFFECT_CHANGE)(AudioContext *p_context, bool p_enabled, ReverbParameters *p_params);

// API
AudioContext *audio_create_context(AudioEngine p_engine);

extern PFN_AUDIO_DESTROY_CONTEXT audio_destroy_context;
extern PFN_AUDIO_CREATE_VOICE audio_create_voice;
extern PFN_AUDIO_VOICE_DESTROY audio_voice_destroy;
extern PFN_AUDIO_VOICE_SET_VOLUME audio_voice_set_volume;
extern PFN_AUDIO_VOICE_SET_FREQUENCY audio_voice_set_frequency;

extern PFN_AUDIO_WAVE_LOAD audio_wave_load;
extern PFN_AUDIO_WAVE_PLAY audio_wave_play;

extern PFN_AUDIO_EFFECT_CHANGE audio_effect_change;

#endif // FAUDIOFILTERDEMO_AUDIO_H
