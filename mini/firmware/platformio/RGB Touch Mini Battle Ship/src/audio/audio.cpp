#include "audio/audio.h"
#include "WiFi.h"
#include "frameworks/mp_game.h"

float hhz = 440;
float tm = 1.0;
float vol = 0.5;

void AudioClass::setup(int8_t pin_data, int8_t pin_bclk, int8_t _pin_lrclk, int8_t _pin_sd_mode)
{
	out = new AudioOutputI2S();
	wav = new AudioGeneratorWAV();

	out->SetPinout(pin_bclk, _pin_lrclk, pin_data);

	pinMode(_pin_sd_mode, OUTPUT);
	digitalWrite(_pin_sd_mode, HIGH);

	wav_files = {
		{"beeps", SFX(voice_beeps, sizeof(voice_beeps))},
		{"begin", SFX(voice_begin, sizeof(voice_begin))},
		{"bounce", SFX(voice_bounce, sizeof(voice_bounce))},
		{"brightness", SFX(voice_brightness, sizeof(voice_brightness))},
		{"cancel", SFX(voice_cancel, sizeof(voice_cancel))},
		{"change", SFX(voice_change, sizeof(voice_change))},
		{"calm", SFX(voice_calm, sizeof(voice_calm))},
		{"done", SFX(voice_done, sizeof(voice_done))},
		{".", SFX(voice_dot, sizeof(voice_dot))},
		{"down", SFX(voice_down, sizeof(voice_down))},
		{"error", SFX(voice_error, sizeof(voice_error))},
		{"error_send", SFX(voice_error_sending, sizeof(voice_error_sending))},
		{"error_recv", SFX(voice_error_recv, sizeof(voice_error_recv))},
		{"found_peer", SFX(voice_found_peer, sizeof(voice_found_peer))},
		{"goodbye", SFX(voice_goodbye, sizeof(voice_goodbye))},
		{"hello", SFX(voice_hello, sizeof(voice_hello))},
		{"in", SFX(voice_in, sizeof(voice_in))},
		{"left", SFX(voice_left, sizeof(voice_left))},
		{"magic_sand", SFX(voice_magic_sand, sizeof(voice_magic_sand))},
		{"menu", SFX(voice_menu, sizeof(voice_menu))},
		{"no", SFX(voice_no, sizeof(voice_no))},
		{"num_8", SFX(voice_num_eight, sizeof(voice_num_eight))},
		{"num_5", SFX(voice_num_five, sizeof(voice_num_five))},
		{"num_4", SFX(voice_num_four, sizeof(voice_num_four))},
		{"num_9", SFX(voice_num_nine, sizeof(voice_num_nine))},
		{"num_1", SFX(voice_num_one, sizeof(voice_num_one))},
		{"num_7", SFX(voice_num_seven, sizeof(voice_num_seven))},
		{"num_6", SFX(voice_num_six, sizeof(voice_num_six))},
		{"num_0", SFX(voice_zero, sizeof(voice_zero))},
		{"num_3", SFX(voice_num_three, sizeof(voice_num_three))},
		{"num_2", SFX(voice_num_two, sizeof(voice_num_two))},
		{"off", SFX(voice_off, sizeof(voice_off))},
		{"ok", SFX(voice_ok, sizeof(voice_ok))},
		{"on", SFX(voice_on, sizeof(voice_on))},
		{"orientation_locked", SFX(voice_orientation_locked, sizeof(voice_orientation_locked))},
		{"orientation_unlocked", SFX(voice_orientation_unlocked, sizeof(voice_orientation_unlocked))},
		{"out", SFX(voice_out, sizeof(voice_out))},
		{"piano", SFX(voice_piano, sizeof(voice_piano))},
		{"play", SFX(voice_play, sizeof(voice_play))},
		{"power_off", SFX(voice_power_off, sizeof(voice_power_off))},
		{"ready", SFX(voice_ready, sizeof(voice_ready))},
		{"record", SFX(voice_record, sizeof(voice_record))},
		{"right", SFX(voice_right, sizeof(voice_right))},
		{"patterns", SFX(voice_patterns, sizeof(voice_patterns))},
		{"running", SFX(voice_running, sizeof(voice_running))},
		{"sand", SFX(voice_sand, sizeof(voice_sand))},
		{"save", SFX(voice_save, sizeof(voice_save))},
		{"saved", SFX(voice_saved, sizeof(voice_saved))},
		{"send", SFX(voice_send, sizeof(voice_send))},
		{"settings", SFX(voice_settings, sizeof(voice_settings))},
		{"setup", SFX(voice_setup, sizeof(voice_setup))},
		{"setup_wifi", SFX(voice_setup_wifi, sizeof(voice_setup_wifi))},
		{"share", SFX(voice_share, sizeof(voice_share))},
		{"start", SFX(voice_start, sizeof(voice_start))},
		{"starting", SFX(voice_starting, sizeof(voice_starting))},
		{"start_draw", SFX(voice_start_drawing, sizeof(voice_start_drawing))},
		{"stopping", SFX(voice_stopping, sizeof(voice_stopping))},
		{"thanks", SFX(voice_thanks, sizeof(voice_thanks))},
		// {"unexpected", SFX(voice_unexpected, sizeof(voice_unexpected))},
		{"um", SFX(voice_um, sizeof(voice_um))},
		{"up", SFX(voice_up, sizeof(voice_up))},
		{"upload", SFX(voice_upload, sizeof(voice_upload))},
		{"volume", SFX(voice_volume, sizeof(voice_volume))},
		{"webserver", SFX(voice_webserver, sizeof(voice_webserver))},
		{"webserver_running", SFX(voice_webserver_running, sizeof(voice_webserver_running))},
		{"wifi", SFX(voice_wifi, sizeof(voice_wifi))},
		{"wifiman", SFX(voice_wifiman, sizeof(voice_wifiman))},
		{"yes", SFX(voice_yes, sizeof(voice_yes))},
	};

	// Populate voice modes
	play_mode_voices = {"play", "play", "beeps", "sand", "piano", "bounce", "patterns", "calm"};
	settings_mode_voices = {"volume", "brightness", "wifi", "orientation"};
}

float AudioClass::sine_wave(const float time)
{
	float v = cos(TWO_PI * hhz * time * tm);
	v *= vol;
	vol = constrain(vol - 0.0001, 0.00, 1.0); // Fade down over time
	return v;
}

void AudioClass::play_tone(float _hz, float _tm)
{
	if (settings.config.volume == 0)
		return;

	hhz = _hz + 660;
	tm = _tm;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)settings.config.volume / audio_player.max_volume) / 6.0;
	if (!wav->isRunning())
	{
		func = new AudioFileSourceFunction(20);
		func->addAudioGenerators([this](const float time) {
			return sine_wave(time);
		});
		wav->begin(func, out);
	}
}

void AudioClass::play_menu_beep(int index, int y)
{
	if (settings.config.volume == 0)
		return;

	hhz = piano_notes[index];
	tm = 1.0;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)settings.config.volume / audio_player.max_volume) / 6.0;
	if (wav->isRunning())
		wav->stop();
	func = new AudioFileSourceFunction(0.25);
	func->addAudioGenerators([this](const float time) {
		return sine_wave(time);
	});
	wav->begin(func, out);
}

float AudioClass::get_piano_key_freq(uint8_t key)
{
	// A4 is the 49th key and has a frequency of 440 Hz
	int A4_keyNumber = 49;
	float A4_frequency = 440.0;

	// Calculate the frequency using the 12-tone equal temperament formula
	float frequency = pow(2.0, (key - A4_keyNumber) / 12.0) * A4_frequency;

	return frequency;
}

void AudioClass::play_note(int index, int y, float vol_fade, float duration)
{
	if (settings.config.volume == 0)
		return;

	if (y < 6)
		index += 12;
	hhz = get_piano_key_freq(52 + index);
	tm = 1.0;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)settings.config.volume / audio_player.max_volume) / 6.0;
	vol *= vol_fade;
	if (!wav->isRunning())
	{
		func = new AudioFileSourceFunction(duration);
		func->addAudioGenerators([this](const float time) {
			return sine_wave(time);
		});
		wav->begin(func, out);
	}
}

void AudioClass::set_volume(uint8_t vol)
{
	out->SetGain((float)vol / max_volume);
}

void AudioClass::find_game_wav(const char *wav_name)
{
	if ( !game )
		return;

	// If we have a game , lets ask it first for wav data
	SFX game_wav = game->get_game_wave_file(wav_name);
	if ( game_wav.array && game_wav.size > 0 )  {
		// We just set class var with data if found
		file = new AudioFileSourcePROGMEM(game_wav.array, game_wav.size);
	}
}

void AudioClass::play_wav(const char *wav_name, bool force)
{
	if (settings.config.volume == 0)
		return;

	find_game_wav(wav_name);

	if ( !file ) {
		file = new AudioFileSourcePROGMEM(wav_files[wav_name].array, wav_files[wav_name].size);
	}

	if ( file ) {
		wav->begin(file, out);
	} else {
		info_printf("play_wav[%s] Not Found\n",wav_name);
	}
}

void AudioClass::play_wav_queue(const char *wav_name)
{
	if (settings.config.volume == 0)
		return;

	if (wav->isRunning())
	{
		info_printf("adding %s to queue\n", wav_name);
		queue.push_back(wav_name);
	}
	else
	{
		info_printf("loading %s to queue\n", wav_name);

		find_game_wav(wav_name);

		if ( !file ) {
			file = new AudioFileSourcePROGMEM(wav_files[wav_name].array, wav_files[wav_name].size);
		}


		if ( file ) {
			wav->begin(file, out);
		} else {
			info_printf("play_wav_queue[%s] Not Found\n",wav_name);
		}
	}
}

void AudioClass::play_wav_char_squence(const String &word)
{
	for (int i = 0; i < word.length(); i++)
	{
		String sound_name = "num_" + word.substring(i, i + 1);
		const char *sound_name_char = sound_name.c_str();

		info_printf("adding %s to queue\n", sound_name.c_str());
		queue.push_back(strdup(sound_name_char));
	}
}

void AudioClass::play_ip_address()
{
	if (settings.config.volume == 0)
		return;

	IPAddress ip = WiFi.localIP();
	String ip_str = ip.toString();

	int i = 0;
	while (i < ip_str.length())
	{
		String sound_name = "num_" + ip_str.substring(i, i + 1);
		const char *sound_name_char = sound_name.c_str();

		if (wav->isRunning())
			continue;

		info_printf("playing %s\n", sound_name_char);

		if (wav_files.find(sound_name_char) != wav_files.end())
		{
			SFX &snd = wav_files[sound_name_char];
			file = new AudioFileSourcePROGMEM(snd.array, snd.size);
			wav->begin(file, out);
		}
		else
		{
			info_println("error snd not found");
		}
		i++;
	}
}

void AudioClass::update()
{
	if (wav->isRunning())
	{
		if (!wav->loop())
			wav->stop();
	}
	else if (!queue.empty())
	{
		if (settings.config.volume == 0)
			return;

		info_printf("playing %s from queue\n", queue.front());
		play_wav(queue.front());
		queue.erase(queue.begin());
	}
}

AudioClass audio_player;