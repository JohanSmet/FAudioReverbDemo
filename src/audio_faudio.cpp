#include "audio.h"

#include <FAudio.h>
#include <FAudioFX.h>

#include "dr_wav.h"

struct AudioContext 
{
	FAudio *faudio;
	bool output_5p1;
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
	ReverbParameters	   reverb_params;
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
	// create reverb effect
	void *xapo = nullptr;
	uint32_t hr = FAudioCreateReverb(&xapo, 0);

	if (hr != 0)
	{
		return nullptr;
	}

	// create effect chain
	p_context->reverb_effect.InitialState = p_context->reverb_enabled;
	p_context->reverb_effect.OutputChannels = p_context->wav_channels;
	p_context->reverb_effect.pEffect = xapo;

	p_context->effect_chain.EffectCount = 1;
	p_context->effect_chain.pEffectDescriptors = &p_context->reverb_effect;

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
	hr = FAudio_CreateSourceVoice(p_context->faudio, &voice, &waveFormat, FAUDIO_VOICE_USEFILTER, FAUDIO_MAX_FREQ_RATIO, NULL, NULL, &p_context->effect_chain);

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
	FAudioVoice_SetEffectParameters(context->voice->voice, 0, &context->reverb_params, sizeof(context->reverb_params), FAUDIO_COMMIT_NOW);
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

void faudio_wave_load(AudioContext *p_context, AudioSampleWave sample, bool stereo)
{
	if (p_context->voice)
	{
		audio_voice_destroy(p_context->voice);
	}

	p_context->wav_samples = drwav_open_and_read_file_f32(
		(!stereo) ? audio_sample_filenames[sample] : audio_stereo_filenames[sample],
		&p_context->wav_channels,
		&p_context->wav_samplerate,
		&p_context->wav_sample_count);

	p_context->wav_sample_count /= p_context->wav_channels;

	p_context->voice = audio_create_voice(p_context, p_context->wav_samples, p_context->wav_sample_count, p_context->wav_samplerate, p_context->wav_channels);
	faudio_reverb_set_params(p_context);
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
	if (p_context->reverb_enabled && !p_enabled)
	{
		FAudioVoice_DisableEffect(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
		p_context->reverb_enabled = p_enabled;
	}
	else if (!p_context->reverb_enabled && p_enabled)
	{
		FAudioVoice_EnableEffect(p_context->voice->voice, 0, FAUDIO_COMMIT_NOW);
		p_context->reverb_enabled = p_enabled;
	}

	p_context->reverb_params = *p_params;
	faudio_reverb_set_params(p_context);
}

AudioContext *faudio_create_context(bool output_5p1)
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

	hr = FAudio_CreateMasteringVoice(faudio, &mastering_voice, output_5p1 ? 6 : 2, 44100, 0, 0, NULL);
	if (hr != 0)
		return nullptr;

	// return a context object
	AudioContext *context = new AudioContext();
	context->faudio = faudio;
	context->output_5p1 = output_5p1;
	context->mastering_voice = mastering_voice;

	context->voice = NULL;
	context->wav_samples = NULL;
	context->reverb_params = { 0 };
	context->reverb_enabled = false;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0, false);

	return context;
}
