#include "main_gui.h"
#include "imgui/imgui.h"

#include "audio_player.h"
#include <math.h>

int next_window_dims(int y_pos, int height)
{
	ImGui::SetNextWindowPos(ImVec2(0, static_cast<float>(y_pos)));
	ImGui::SetNextWindowSize(ImVec2(640, static_cast<float>(height)));

	return y_pos + height;
}

void main_gui()
{
	bool update_engine = false;
	bool update_wave = false;
	bool play_wave = false;
	bool update_effect = false;
	bool update_reverb = false;

	// gui
	int window_y = next_window_dims(0, 50);
	ImGui::Begin("Output Audio Engine");

		static int audio_engine = (int)AudioEngine_FAudio;
		update_engine |= ImGui::RadioButton("FAudio", &audio_engine, (int)AudioEngine_FAudio);
		#ifdef HAVE_XAUDIO2 
		ImGui::SameLine();
		update_engine |= ImGui::RadioButton("XAudio2", &audio_engine, (int)AudioEngine_XAudio2); 
		#endif

	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Wave file to play");

		static int wave_index = (int)AudioWave_SnareDrum01;
		update_wave |= ImGui::RadioButton("Snare Drum (Forte)", &wave_index, (int)AudioWave_SnareDrum01); ImGui::SameLine();
		update_wave |= ImGui::RadioButton("Snare Drum (Fortissimo)", &wave_index, (int)AudioWave_SnareDrum02); ImGui::SameLine();
		update_wave |= ImGui::RadioButton("Snare Drum (Mezzo-Forte)", &wave_index, (int)AudioWave_SnareDrum03); 

		play_wave = ImGui::Button("Play");
		
	ImGui::End();

	window_y = next_window_dims(window_y, 80);
	ImGui::Begin("Reverb effect");
		
		static bool effect_enabled = false;
		update_effect |= ImGui::Checkbox("Enabled", &effect_enabled);
		
		static int preset_index = -1;
		static ReverbI3DL2Parameters reverb_params = audio_reverb_presets[0];

		if (ImGui::Combo("Preset", &preset_index, audio_reverb_preset_names, audio_reverb_preset_count)) {
			reverb_params = audio_reverb_presets[preset_index];
			update_effect = true;
		}

	ImGui::End();


	// audio control
	static AudioPlayer	player;

	if (update_engine)
	{
		player.shutdown();
		player.setup((AudioEngine)audio_engine);
	}

	if (update_wave | update_engine)
	{
		player.load_wave_sample((AudioSampleWave) wave_index);
	}

	if (play_wave) {
		player.play_wave();
	}

	if (update_effect | update_wave)
	{
		player.change_effect(effect_enabled, &reverb_params);
	}
}