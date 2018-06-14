#ifndef FAUDIOFILTERDEMO_AUDIO_PLAYER_H
#define FAUDIOFILTERDEMO_AUDIO_PLAYER_H

#include "audio.h"

class AudioPlayer
{
	public :
		AudioPlayer() 
		{
			setup(AudioEngine_FAudio, false);
		}

		void setup(AudioEngine p_engine, bool output_5p1)
		{
			m_context = audio_create_context(p_engine, output_5p1);
		}

		void shutdown()
		{
			if (m_context == nullptr)
				return;

			audio_destroy_context(m_context);
			m_context = nullptr;
		}
		
		void load_wave_sample(AudioSampleWave sample, bool stereo)
		{
			if (m_context == nullptr)
				return;
			
			audio_wave_load(m_context, sample, stereo);
		}

		void play_wave()
		{
			if (m_context == nullptr)
				return;
			
			audio_wave_play(m_context);
		}

		void change_effect(bool p_enabled, ReverbParameters *p_params)
		{
			if (m_context == nullptr)
				return;

			audio_effect_change(m_context, p_enabled, p_params);
		}

	private : 
		AudioContext *	m_context;
};

#endif // FAUDIOFILTERDEMO_AUDIO_PLAYER_H