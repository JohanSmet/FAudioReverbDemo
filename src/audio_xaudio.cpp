#include "audio.h"

#include <xaudio2.h>
#include <xaudio2fx.h>
#include "dr_wav.h"

struct AudioContext 
{
	IXAudio2 *xaudio2;
	IXAudio2MasteringVoice *mastering_voice;

	unsigned int wav_channels;
	unsigned int wav_samplerate;
	drwav_uint64 wav_sample_count;
	float *		 wav_samples;

	struct AudioVoice *voice;
	XAUDIO2_BUFFER    buffer;

	XAUDIO2_EFFECT_DESCRIPTOR reverb_effect;
	XAUDIO2_EFFECT_CHAIN	  effect_chain;
	ReverbParameters		  reverb_params;
	bool					  reverb_enabled;
};

struct AudioVoice
{
	AudioContext *context;
	IXAudio2SourceVoice *voice;
};

struct AudioFilter
{
	AudioContext *context;
	XAUDIO2_FILTER_PARAMETERS params;
};

void xaudio_destroy_context(AudioContext *p_context)
{
	p_context->mastering_voice->DestroyVoice();
	p_context->xaudio2->Release();
	delete p_context;
}

AudioVoice *xaudio_create_voice(AudioContext *p_context, float *p_buffer, size_t p_buffer_size, int p_sample_rate, int p_num_channels)
{
	// create a source voice
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	waveFormat.nChannels = p_num_channels;
	waveFormat.nSamplesPerSec = p_sample_rate;
	waveFormat.nBlockAlign = p_num_channels * 4;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.wBitsPerSample = 32;
	waveFormat.cbSize = 0;

	IXAudio2SourceVoice *voice;
	HRESULT hr = p_context->xaudio2->CreateSourceVoice(&voice, &waveFormat, XAUDIO2_VOICE_USEFILTER, XAUDIO2_MAX_FREQ_RATIO);

	if (FAILED(hr)) {
		return nullptr;
	}
	
	voice->SetVolume(1.0f);

	// submit the array
	p_context->buffer = { 0 };
	p_context->buffer.AudioBytes = 4 * p_buffer_size * p_num_channels;
	p_context->buffer.pAudioData = (byte *)p_buffer;
	p_context->buffer.Flags = XAUDIO2_END_OF_STREAM;
	p_context->buffer.PlayBegin = 0;
	p_context->buffer.PlayLength = p_buffer_size;
	p_context->buffer.LoopBegin = 0;
	p_context->buffer.LoopLength = 0;
	p_context->buffer.LoopCount = 0;

	// return a voice struct
	AudioVoice *result = new AudioVoice();
	result->context = p_context;
	result->voice = voice;
	return result;
}

void xaudio_reverb_set_params(AudioContext *context)
{
/*	XAUDIO2FX_REVERB_PARAMETERS native_params = { 0 };

	ReverbConvertI3DL2ToNative((XAUDIO2FX_REVERB_I3DL2_PARAMETERS *)&context->reverb_params, &native_params);*/
	HRESULT hr = context->voice->voice->SetEffectParameters(
		0, 
		&context->reverb_params, 
		sizeof(XAUDIO2FX_REVERB_PARAMETERS));
}

void xaudio_create_reverb(AudioVoice *voice)
{
	// create reverb effect
	IUnknown *xapo = nullptr;
	HRESULT hr = XAudio2CreateReverb(&xapo);

	if (FAILED(hr))
		return;

	// create effect chain
	voice->context->reverb_effect.InitialState = voice->context->reverb_enabled;
	voice->context->reverb_effect.OutputChannels = voice->context->wav_channels;
	voice->context->reverb_effect.pEffect = xapo;

	voice->context->effect_chain.EffectCount = 1;
	voice->context->effect_chain.pEffectDescriptors = &voice->context->reverb_effect;

	// attach to voice
	hr = voice->voice->SetEffectChain(&voice->context->effect_chain);
	if (FAILED(hr))
		return;
	xapo->Release();

	// set initial parameters
	xaudio_reverb_set_params(voice->context);
}

void xaudio_voice_destroy(AudioVoice *p_voice)
{
	drwav_free(p_voice->context->wav_samples);
	p_voice->voice->DestroyVoice();
}

void xaudio_voice_set_volume(AudioVoice *p_voice, float p_volume)
{
	p_voice->voice->SetVolume(p_volume);
}

void xaudio_voice_set_frequency(AudioVoice *p_voice, float p_frequency)
{
	p_voice->voice->SetFrequencyRatio(p_frequency);
}

void xaudio_wave_load(AudioContext *p_context, AudioSampleWave sample)
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
	xaudio_create_reverb(p_context->voice);
}

void xaudio_wave_play(AudioContext *p_context)
{
	p_context->voice->voice->Stop();
	p_context->voice->voice->FlushSourceBuffers();

	HRESULT hr = p_context->voice->voice->SubmitSourceBuffer(&p_context->buffer);

	if (FAILED(hr)) {
		return;
	}

	p_context->voice->voice->Start();
}

void xaudio_effect_change(AudioContext *p_context, bool p_enabled, ReverbParameters *p_params)
{
	HRESULT hr;

	if (p_context->reverb_enabled && !p_enabled)
	{
		hr = p_context->voice->voice->DisableEffect(0);
		p_context->reverb_enabled = p_enabled;
	}
	else if (!p_context->reverb_enabled && p_enabled)
	{
		hr = p_context->voice->voice->EnableEffect(0);
		p_context->reverb_enabled = p_enabled;
	}

	p_context->reverb_params = *p_params;
	xaudio_reverb_set_params(p_context);
}


AudioContext *xaudio_create_context()
{
	// setup function pointers
	audio_destroy_context = xaudio_destroy_context;
	audio_create_voice = xaudio_create_voice;
	audio_voice_destroy = xaudio_voice_destroy;
	audio_voice_set_volume = xaudio_voice_set_volume;
	audio_voice_set_frequency = xaudio_voice_set_frequency;

	audio_wave_load = xaudio_wave_load;
	audio_wave_play = xaudio_wave_play;

	audio_effect_change = xaudio_effect_change;

	// create XAudio object
	IXAudio2 *xaudio2;

	HRESULT hr = XAudio2Create(&xaudio2);
	if (FAILED(hr))
		return nullptr;

	// create a mastering voice
	IXAudio2MasteringVoice *mastering_voice;

	hr = xaudio2->CreateMasteringVoice(&mastering_voice, 2);
	if (FAILED(hr))
		return nullptr;

	// return a context object
	AudioContext *context = new AudioContext();
	context->xaudio2 = xaudio2;
	context->mastering_voice = mastering_voice;
	context->voice = NULL;
	context->wav_samples = NULL;
	context->reverb_params = audio_reverb_presets[0];
	context->reverb_enabled = false;

	// load the first wave
	audio_wave_load(context, (AudioSampleWave) 0);

	return context;
}
