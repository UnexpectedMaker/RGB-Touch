#pragma once

#include <vector>
#include "Arduino.h"
#include "WiFi.h"
#include "esp_wifi.h"
#include "web/wifi_controller.h"

struct RecordedPoint
{
		int x;
		int y;
		uint8_t fade_amount = 0;
		uint8_t color = 0;
		uint16_t created = 0;
		bool touched = false;

		RecordedPoint(int px, int py, uint8_t col, bool retouch, uint16_t tm) : x(px), y(py), fade_amount(0), color(col), touched(retouch), created(tm) {}

		String as_string()
		{
			return String(x) + "," + String(y) + (touched ? "T" : " ") + String(color) + "_" + String(created);
		}
};

struct Recording
{
		uint8_t fade_step = 15;
		unsigned long start_time = 0;
		unsigned long playback_start_time = 0;
		int playback_index = 0;
		std::vector<RecordedPoint> touch_recording;

		Recording() {}
		Recording(uint8_t fade) : fade_step(fade)
		{
			start_time = millis();
		}
};

class Recorder
{
	public:
		Recorder()
		{
		}

		bool is_recording = false;
		bool is_playing = false;
		bool is_receiving = false;

		unsigned long receive_timout = 0;
		uint16_t max_recording_time = 10000; // 10 seconds

		uint8_t max_chunks = 20;

		Recording rec;

		bool share_recording();
		bool recieve_recording();
		void save();
		void load();

		void printPacket();

		IPAddress target_ip;
};

extern Recorder recorder;