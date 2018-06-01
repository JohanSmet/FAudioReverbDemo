#include "audio.h"

#include <FAudio.h>
#include <FAudioFX.h>

#include "dr_wav.h"

struct AudioContext 
{
	FAudio *faudio;
	FAudioMasteringVoice *mastering_voice;

	unsigned int wav_channels;
	unsigned int wav_samplerate;
	drwav_uint64 wav_sample_count;
	float *		 wav_samples;

	struct AudioVoice *voice;
	FAudioBuffer      buffer;
	FAudioBuffer	  silence;

	FAudioEffectDescriptor reverb_effect;
	FAudioEffectChain	   effect_chain;
	ReverbTestParameters   reverb_params;
	bool				   reverb_enabled;
};

struct AudioVoice
{
	AudioContext *context;
	FAudioSourceVoice *voice;
};

struct AudioFilter
{
	AudioContext *context;
	FAudioFilterParameters params;
};

void faudio_destroy_context(AudioContext *p_context)
{
	FAudioVoice_DestroyVoice(p_context->mastering_voice);
	// FAudioDestroy(p_context->faudio);
	delete p_context;
}

AudioVoice *faudio_create_voice(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels)
{
	// create a source voice
	FAudioWaveFormatEx waveFormat;
	waveFormat.wFormatTag = 3;
	waveFormat.nChannels = p_num_channels;
	waveFormat.nSamplesPerSec = p_sample_rate;
	waveFormat.nAvgBytesPerSec = p_sample_rate * 4;
	waveFormat.nBlockAlign = p_num_channels * 4;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	FAudioSourceVoice *voice;
	uint32_t hr = FAudio_CreateSourceVoice(p_context->faudio, &voice, &waveFormat, FAUDIO_VOICE_USEFILTER, FAUDIO_MAX_FREQ_RATIO, NULL, NULL, NULL);

	if (hr != 0) {
		return nullptr;
	}

	FAudioVoice_SetVolume(voice, 1.0f, FAUDIO_COMMIT_NOW);
	
	// submit the array
	p_context->buffer = { 0 };
	p_context->buffer.AudioBytes = 4 * p_buffer_size * p_num_channels;
	p_context->buffer.pAudioData = (uint8_t *)p_buffer;
	p_context->buffer.Flags = FAUDIO_END_OF_STREAM;
	p_context->buffer.PlayBegin = 0;
	p_context->buffer.PlayLength = p_buffer_size;
	p_context->buffer.LoopBegin = 0;
	p_context->buffer.LoopLength = 0;
	p_context->buffer.LoopCount = 0;

	
	size_t silence_len = 2 * 48000 * p_num_channels;
	float *silence_buffer = new float[silence_len];
	for (uint32_t i = 0; i < silence_len; ++i)
	{
		silence_buffer[i] = 0.0f;
	}

	p_context->silence = { 0 };
	p_context->silence.AudioBytes = 4 * silence_len;
	p_context->silence.pAudioData = (uint8_t *)silence_buffer;
	p_context->silence.Flags = FAUDIO_END_OF_STREAM;
	p_context->silence.PlayBegin = 0;
	p_context->silence.PlayLength = silence_len / p_num_channels;
	p_context->silence.LoopBegin = 0;
	p_context->silence.LoopLength = 0;
	p_context->silence.LoopCount = 0;


	// return a voice struct
	AudioVoice *result = new AudioVoice();
	result->context = p_context;
	result->voice = voice;
	return result;
}

void faudio_reverb_set_params(AudioContext *context)
{
/*	FAudioFXReverbParameters native_params = { 0 };

	ReverbConvertI3DL2ToNative((FAudioFXReverbI3DL2Parameters *)&context->reverb_params, &native_params);
	uint32_t hr = FAudioVoice_SetEffectParameters(context->voice->voice, 0, &native_params, sizeof(native_params), FAUDIO_COMMIT_NOW);
	*/
	uint32_t hr = FAudioVoice_SetEffectParameters(context->voice->voice, 0, &context->reverb_params, sizeof(context->reverb_params), FAUDIO_COMMIT_NOW);
}

void faudio_create_reverb(AudioVoice *voice)
{
	// create reverb effect
	void *xapo = nullptr;
	uint32_t hr = FAudioCreateReverb(&xapo, 0);

	if (hr != 0)
		return;

	// create effect chain
	voice->context->reverb_effect.InitialState = voice->context->reverb_enabled;
	voice->context->reverb_effect.OutputChannels = voice->context->wav_channels;
	voice->context->reverb_effect.pEffect = xapo;

	voice->context->effect_chain.EffectCount = 1;
	voice->context->effect_chain.pEffectDescriptors = &voice->context->reverb_effect;

	// attach to voice
	FAudioVoice_SetEffectChain(voice->voice, &voice->context->effect_chain);

	// set initial parameters
	faudio_reverb_set_params(voice->context);
}

void faudio_voice_destroy(AudioVoice *p_voice)
{
	FAudioVoice_DestroyVoice(p_voice->voice);
}

void faudio_voice_set_volume(AudioVoice *p_voice, float p_volume)
{
	FAudioVoice_SetVolume(p_voice->voice, p_volume, FAUDIO_COMMIT_NOW);
}

void faudio_voice_set_frequency(AudioVoice *p_voice, float p_frequency)
{
	FAudioSourceVoice_SetFrequencyRatio(p_voice->voice, p_frequency, FAUDIO_COMMIT_NOW);
}

void faudio_wave_load(AudioContext *p_context, AudioSampleWave sample)
{
	if (p_context->voice)
	{
		audio_voice_destroy(p_context->voice);
	}

	p_context->wav_samples = drwav_open_and_read_file_f32(
		audio_sample_filenames[sample],
		&p_context->wav_channels,
		&p_context->wav_samplerate,
		&p_context->wav_sample_count);

	p_context->wav_sample_count /= p_context->wav_channels;

	p_context->voice = audio_create_voice(p_context, p_context->wav_samples, p_context->wav_sample_count, p_context->wav_samplerate, p_context->wav_channels);
	faudio_create_reverb(p_context->voice);
}

void faudio_wave_play(AudioContext *p_context)
{
	FAudioSourceVoice_Stop(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
	FAudioSourceVoice_FlushSourceBuffers(p_context->voice->voice);

	FAudioSourceVoice_SubmitSourceBuffer(p_context->voice->voice, &p_context->buffer, NULL);
	FAudioSourceVoice_SubmitSourceBuffer(p_context->voice->voice, &p_context->silence, NULL);
	FAudioSourceVoice_Start(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
}

void faudio_effect_change(AudioContext *p_context, bool p_enabled, ReverbParameters *p_params)
{
	uint32_t hr;

	if (p_context->reverb_enabled && !p_enabled)
	{
		hr = FAudioVoice_DisableEffect(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
		p_context->reverb_enabled = p_enabled;
	}
	else if (!p_context->reverb_enabled && p_enabled)
	{
		hr = FAudioVoice_EnableEffect(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
		p_context->reverb_enabled = p_enabled;
	}

	p_context->reverb_params = *((ReverbTestParameters *) p_params);
	faudio_reverb_set_params(p_context);
}

AudioContext *faudio_create_context()
{
	// setup function pointers
	audio_destroy_context = faudio_destroy_context;
	audio_create_voice = faudio_create_voice;
	audio_voice_destroy = faudio_voice_destroy;
	audio_voice_set_volume = faudio_voice_set_volume;
	audio_voice_set_frequency = faudio_voice_set_frequency;

	audio_wave_load = faudio_wave_load;
	audio_wave_play = faudio_wave_play;

	audio_effect_change = faudio_effect_change;

	// create Faudio object
	FAudio *faudio;

	uint32_t hr = FAudioCreate(&faudio, 0, FAUDIO_DEFAULT_PROCESSOR);
	if (hr != 0)
		return nullptr;

	// create a mastering voice
	FAudioMasteringVoice *mastering_voice;

	hr = FAudio_CreateMasteringVoice(faudio, &mastering_voice, 2, 44100, 0, 0, NULL);
	if (hr != 0)
		return nullptr;

	// return a context object
	AudioContext *context = new AudioContext();
	context->faudio = faudio;
	context->mastering_voice = mastering_voice;

	context->voice = NULL;
	context->wav_samples = NULL;
	context->reverb_params = { 0 };
	context->reverb_enabled = false;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0);

	return context;
}
