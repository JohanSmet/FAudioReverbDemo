#ifndef FAUDIOFILTERDEMO_AUDIO_H
#define FAUDIOFILTERDEMO_AUDIO_H

#include <stddef.h>

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

extern const char *audio_sample_filenames[];
extern const char *audio_reverb_preset_names[];
extern const ReverbI3DL2Parameters audio_reverb_presets[];
extern const size_t				   audio_reverb_preset_count;

typedef void(*PFN_AUDIO_DESTROY_CONTEXT)(AudioContext *p_context);

typedef AudioVoice *(*PFN_AUDIO_CREATE_VOICE)(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels);
typedef void (*PFN_AUDIO_VOICE_DESTROY)(AudioVoice *p_voice);
typedef void (*PFN_AUDIO_VOICE_SET_VOLUME)(AudioVoice *p_voice, float p_volume);
typedef void (*PFN_AUDIO_VOICE_SET_FREQUENCY)(AudioVoice *p_vioce, float p_frequency);

typedef void (*PFN_AUDIO_WAVE_LOAD)(AudioContext *p_context, AudioSampleWave sample);
typedef void (*PFN_AUDIO_WAVE_PLAY)(AudioContext *p_context);

typedef void(*PFN_AUDIO_EFFECT_CHANGE)(AudioContext *p_context, bool p_enabled, ReverbI3DL2Parameters *p_params);

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
