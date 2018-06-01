#ifndef FAUDIOFILTERDEMO_AUDIO_PLAYER_H
#define FAUDIOFILTERDEMO_AUDIO_PLAYER_H

#include "audio.h"

class AudioPlayer
{
	public :
		AudioPlayer() 
		{
			setup(AudioEngine_FAudio);
		}

		void setup(AudioEngine p_engine)
		{
			m_context = audio_create_context(p_engine);
		}

		void shutdown()
		{
			if (m_context == nullptr)
				return;

			audio_destroy_context(m_context);
			m_context = nullptr;
		}
		
		void load_wave_sample(AudioSampleWave sample)
		{
			if (m_context == nullptr)
				return;
			
			audio_wave_load(m_context, sample);
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

		void change_effect_test(bool p_enabled, ReverbTestParameters *p_params)
		{
			if (m_context == nullptr)
				return;

			audio_effect_change(m_context, p_enabled, (ReverbParameters *) p_params);
		}


	private : 
		AudioContext *	m_context;
};

#endif // FAUDIOFILTERDEMO_AUDIO_PLAYER_H