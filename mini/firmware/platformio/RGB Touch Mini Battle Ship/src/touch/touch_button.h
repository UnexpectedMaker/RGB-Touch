#pragma once

#include <vector>
#include <array>
#include <map>
#include <functional>
#include "Arduino.h"
#include "main.h"
#include "display/display.h"

class TouchButton
{
		using CallbackFunction = bool (*)();
		// using DrawIconFunction = void (*)(uint8_t x, uint8_t y);

	public:
		TouchButton(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t duration, CallbackFunction _callback, Modes for_mode = Modes::MENU) : _x(x), _y(y), _w(w), _h(h), _duration(duration), callback(_callback)
		{
			active_in.push_back(for_mode);
		}

		// void setup(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t duration, CallbackFunction _callback);
		bool check_bounds(uint8_t x, uint8_t y);
		bool press();
		void reset();
		void set_icon(ICON *_icon);
		void add_extra_mode(Modes new_mode);
		void draw_icon(Modes m);
		void move_icon(uint8_t delta_x, uint8_t delta_y);

		bool check_active_in(Modes mode);

		std::vector<Modes> active_in;

	private:
		uint8_t _x = 0;
		uint8_t _y = 0;
		uint8_t _w = 0;
		uint8_t _h = 0;
		uint16_t _duration = 0;

		CallbackFunction callback;
		ICON *icon = nullptr;

		unsigned long first_touch = 0;
		unsigned long last_trigger = 0;
};
