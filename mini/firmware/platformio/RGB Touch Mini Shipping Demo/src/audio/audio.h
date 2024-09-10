#pragma once

#include <map>
#include "Arduino.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioFileSourceFunction.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"

// #include "audio/voice/voice_adjust.h"
#include "audio/voice/voice_beeps.h"
#include "audio/voice/voice_begin.h"
#include "audio/voice/voice_bounce.h"
#include "audio/voice/voice_brightness.h"
#include "audio/voice/voice_cancel.h"
#include "audio/voice/voice_calm.h"
#include "audio/voice/voice_change.h"
// #include "audio/voice/voice_clock.h"
#include "audio/voice/voice_done.h"
#include "audio/voice/voice_dot.h"
#include "audio/voice/voice_down.h"
#include "audio/voice/voice_error.h"
#include "audio/voice/voice_error_sending.h"
#include "audio/voice/voice_error_recv.h"
#include "audio/voice/voice_found_peer.h"
#include "audio/voice/voice_goodbye.h"
#include "audio/voice/voice_hello.h"
#include "audio/voice/voice_in.h"
#include "audio/voice/voice_left.h"
#include "audio/voice/voice_magic_sand.h"
#include "audio/voice/voice_menu.h"
#include "audio/voice/voice_no.h"
#include "audio/voice/voice_num_eight.h"
#include "audio/voice/voice_num_five.h"
#include "audio/voice/voice_num_four.h"
#include "audio/voice/voice_num_nine.h"
#include "audio/voice/voice_num_one.h"
#include "audio/voice/voice_num_seven.h"
#include "audio/voice/voice_num_six.h"
#include "audio/voice/voice_num_ten.h"
#include "audio/voice/voice_num_three.h"
#include "audio/voice/voice_num_two.h"
#include "audio/voice/voice_off.h"
#include "audio/voice/voice_ok.h"
#include "audio/voice/voice_on.h"
#include "audio/voice/voice_orientation_locked.h"
#include "audio/voice/voice_orientation_unlocked.h"
#include "audio/voice/voice_out.h"
#include "audio/voice/voice_piano.h"
#include "audio/voice/voice_play.h"
#include "audio/voice/voice_power_off.h"
#include "audio/voice/voice_ready.h"
#include "audio/voice/voice_record.h"
#include "audio/voice/voice_right.h"
#include "audio/voice/voice_patterns.h"
#include "audio/voice/voice_running.h"
#include "audio/voice/voice_sand.h"
#include "audio/voice/voice_save.h"
#include "audio/voice/voice_saved.h"
#include "audio/voice/voice_send.h"
#include "audio/voice/voice_settings.h"
#include "audio/voice/voice_setup.h"
#include "audio/voice/voice_setup_wifi.h"
#include "audio/voice/voice_share.h"
#include "audio/voice/voice_start.h"
#include "audio/voice/voice_starting.h"
#include "audio/voice/voice_start_draw.h"
#include "audio/voice/voice_stopping.h"
#include "audio/voice/voice_thanks.h"
// #include "audio/voice/voice_unexpected.h"
#include "audio/voice/voice_unexpectedmaker.h"
#include "audio/voice/voice_up.h"
#include "audio/voice/voice_upload.h"
#include "audio/voice/voice_volume.h"
#include "audio/voice/voice_webserver.h"
#include "audio/voice/voice_webserver_running.h"
#include "audio/voice/voice_wifi.h"
#include "audio/voice/voice_wifiman.h"
#include "audio/voice/voice_yes.h"
#include "audio/voice/voice_zero.h"

#include "audio/sfx/sfx_button.h"

#include "settings.h"

struct SFX
{
		const int8_t *array;
		uint16_t size;

		SFX() : array(nullptr), size(0) {}
		SFX(const int8_t *_array, uint16_t _size) : array(_array), size(_size) {}
};

class AudioClass
{

	public:
		AudioClass()
		{
		}

		static float hz;

		float max_volume = 40.0;

		void setup(int8_t pin_data, int8_t pin_bclk, int8_t _pin_lrclk, int8_t _pin_sd_mode);
		void play_wav(const char *wav_name, bool force = false);
		void play_wav_queue(const char *wav_name);
		void play_wav_char_squence(const String &word);
		void play_ip_address();
		void play_tone(float _hz, float _tm);
		void play_note(int index, int y, float vol_fade = 1.0, float duration = 20.0);
		float get_piano_key_freq(uint8_t key);
		void play_menu_beep(int index, int y);
		void update();
		void set_volume(uint8_t vol);
		static float sine_wave(const float time);

		inline bool queue_is_empty()
		{
			return queue.empty();
		}

		std::vector<const char *> settings_mode_voices;
		std::vector<const char *> play_mode_voices;

	private:
		AudioGeneratorWAV *wav;
		AudioFileSourcePROGMEM *file;
		AudioOutputI2S *out;

		AudioFileSourceFunction *func;

		std::vector<const char *> queue;

		std::map<const char *, SFX> wav_files;

		std::vector<float> piano_notes = {
			1046.50,
			1108.73,
			1174.66,
			1244.51,
			1318.51,
			1396.91,
			1479.98,
			1567.98,
			1661.22,
			1760.00,
			1864.66,
			1975.53
		};
};

extern AudioClass audio_player;