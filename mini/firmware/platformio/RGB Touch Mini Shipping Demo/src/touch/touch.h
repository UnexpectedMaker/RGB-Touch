#pragma once

#include <vector>
#include <array>
#include <map>

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

#include "touch/touch_button.h"
#include "Adafruit_MPR121.h"
#include "main.h"
#include "recorder.h"

constexpr int MATRIX_SIZE = 12;
constexpr float MIN_MOVE_THRESHOLD = 0.6; // Distance to consider a touch as not moved
constexpr float MAX_MOVE_THRESHOLD = 1.6; // Distance to consider a touch as moved
constexpr int SENSITIVITY_BUFFER = 0;
constexpr int USE_SENSITIVITY_BUFFER = 0;

extern Adafruit_MPR121 cap_col;
extern Adafruit_MPR121 cap_row;

enum SWIPE
{
	NA,
	UP,
	RIGHT,
	DOWN,
	LEFT,
};

struct Point
{
		int x;
		int y;
		uint8_t fade_amount = 0;
		uint8_t color = 0;
		bool _is_dead = false;
		unsigned long lifetime = 0;
		unsigned long created = 0;

		uint16_t filter_x = 0;
		uint16_t filter_y = 0;

		Point() : x(-1), y(-1) {}

		Point(int px, int py, uint8_t col) : x(px), y(py), fade_amount(0), color(col), _is_dead(false)
		{
			lifetime = millis() + 1200;
		}

		Point(int px, int py, uint8_t col, uint16_t f_x, uint16_t f_y) : x(px), y(py), fade_amount(0), color(col), _is_dead(false), filter_x(f_x), filter_y(f_y)
		{
			lifetime = millis() + 1200;
		}

		bool is_dead()
		{
			// info_printf("lifetime %d is dead? %d %d millis() %d lifetime %d \n", (lifetime - millis()), _is_dead, (millis() > lifetime), millis(), lifetime);
			return (_is_dead || (millis() > lifetime));
		}

		void Retouch(uint8_t col)
		{
			color = col;
			fade_amount = 0;
			_is_dead = false;
			lifetime = millis() + 2000;
		}

		void FadeStep(uint8_t amt)
		{
			if ((int16_t)fade_amount + (int16_t)amt >= 255)
				_is_dead = true;
			fade_amount = constrain(fade_amount + amt, 0, 255);
		}

		bool same(const Point &other) const
		{
			return (x == other.x && y == other.y);
		}

		bool isSameRowOrColumn(const Point &other) const
		{
			return ((x == other.x || y == other.y) && !same(other));
		}
};

struct Ripple
{
		uint8_t x;
		uint8_t y;
		uint8_t r;
		uint16_t a;
		uint8_t c;
		bool round = true;
		TouchButton *button = nullptr;

		void grow(uint8_t _a, uint8_t _r)
		{
			a = constrain(a + _a, 0, 260);
			r += _r;
		}

		void press_button()
		{
			if (button != nullptr)
				button->press();
		}
};

class TouchMatrix
{
	public:
		TouchMatrix()
		{
		}

		bool initialise_mpr();
		void setup_buttons();
		bool process_touches(uint8_t col);
		void block_touch_input(bool block);
		void clear_all_touches();

		SWIPE did_swipe();
		void allow_swipes(bool horiz, bool vert);
		bool can_swipe();
		void bump_next_touch(unsigned long amount);

		bool matrix[MATRIX_SIZE][MATRIX_SIZE] = {false};
		std::vector<Point> touches;
		std::vector<Ripple> PATTERNS;
		std::vector<TouchButton> buttons;
		std::vector<Point> swipe_touches;

		unsigned long last_touch = 0;

	private:
		int nextTouchId = 0;
		bool block_touch = false;
		bool can_swipe_h = false;
		bool can_swipe_v = false;

		int16_t touch_start_x;
		int16_t touch_start_y;
		int16_t touch_last_x;
		int16_t touch_last_y;
		unsigned long touch_start_delay = 0;
		bool touch_start_begin = false;

		int16_t default_Filter[2][12] = {0};

		int16_t default_matrix_Filter[12][12] = {0};

		uint16_t currtouched_cols;
		uint16_t currtouched_rows;

		unsigned long touch_timer = 0;
		unsigned long next_touch = 0;

		bool readCell(int x, int y);
		bool readColumn(int x);
		bool readRow(int y);
		void create_ripple(uint8_t x, uint8_t y, uint8_t c, bool round, TouchButton *button);
};

extern TouchMatrix touch;
