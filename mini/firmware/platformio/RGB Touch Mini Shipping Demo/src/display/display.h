#pragma once

#include <map>
#include <Adafruit_GFX.h>		// Library Manager
#include <Adafruit_PixelDust.h> // For sand simulation
#include <Fonts/TomThumb.h>
// #include <Fonts/Tiny3x3a2pt7b.h>

#include <FastLED_NeoMatrix.h>
#include <FastLED.h>

#include "icons.h"
#include "main.h"
#include "settings.h"

constexpr int NUM_LEDS = 12 * 12;

#define MATRIX_DATA 39
#define MATRIX_PWR 38
#define delay FastLED.delay

#define MAX_FPS 45 // Maximum redraw rate, frames/second for magic sand
#define N_COLORS 4
#define BOX_HEIGHT 6
#define N_GRAINS (BOX_HEIGHT * BOX_HEIGHT - 3)

struct BALL
{
		int8_t _x[4] = {0, 0, 0, 0};
		int8_t _y[4] = {0, 0, 0, 0};
		int8_t _dx;
		int8_t _dy;
		uint8_t fade;
		float vol_fade;
		uint8_t bounces;
		uint8_t col;
		uint8_t trail_length = 0;
		bool active;

		float speed;
		float length;

		BALL(uint8_t s_x, uint8_t s_y, int8_t d_x, int8_t d_y, uint8_t _col) : _dx(-d_x), _dy(-d_y), col(_col)
		{
			trail_length = 0;
			_x[0] = s_x;
			_y[0] = s_y;
			fade = 0;
			bounces = 0;
			speed = 2.0;
			length = sqrt(_dx * _dx + _dy * _dy);
			active = true;
			vol_fade = 1.0;
		}

		bool move()
		{
			bool bounce = false;
			if (_x[0] <= 0 || _x[0] == 11)
			{
				_dx = -_dx;
				bounce = true;
			}

			if (_y[0] == 0 || _y[0] == 11)
			{
				_dy = -_dy;
				bounce = true;
			}

			if (trail_length < 3)
				trail_length++;

			// Normalize the direction vector to get the unit vector
			float unit_dx = _dx / length;
			float unit_dy = _dy / length;

			// Scale the unit direction vector by the speed
			float scaled_dx = unit_dx * speed;
			float scaled_dy = unit_dy * speed;

			_x[3] = _x[2];
			_y[3] = _y[2];

			_x[2] = _x[1];
			_y[2] = _y[1];

			_x[1] = _x[0];
			_y[1] = _y[0];

			// Calculate the new position on the grid
			_x[0] = constrain(round(_x[0] + scaled_dx), 0, 11);
			_y[0] = constrain(round(_y[0] + scaled_dy), 0, 11);

			if (bounce)
			{
				bounces++;
				fade = constrain(fade + 20, 0, 255);
				vol_fade = constrain(vol_fade - 0.05, 0.0, 1.0);
				if (fade == 255)
					active = false;
			}

			return bounce;
		}
};

struct ICON
{
		std::vector<const uint16_t *> frames;
		// uint8_t size;
		uint8_t width;
		uint8_t height;
		uint8_t current_frame = 0;
		uint8_t counter = 0;

		ICON() : width(0), height(0)
		{
		}
		ICON(const uint16_t *frame, uint8_t _w, uint8_t _h) : width(_w), height(_h)
		{
			frames.push_back(frame);
		}

		void Add_Frame(const uint16_t *frame)
		{
			frames.push_back(frame);
		}

		void cycle_frame()
		{
			counter++;
			if (counter == 8)
			{
				counter = 0;
				current_frame++;
				if (current_frame == frames.size())
					current_frame = 0;
			}
		}
};

class MatrixDisplay
{
	public:
		uint8_t color_index = 0;

		uint8_t current_touch_color = 0;

		unsigned long next_redraw = 0;
		uint8_t next_redraw_step = 10;
		uint8_t fade_step = 15;

		uint8_t target_brightness = 0;
		uint8_t current_brightness = 0;
		uint8_t min_brightness = 10;
		uint8_t max_brightness = 250;
		bool display_powered = false;

		void begin();
		void fill_screen(uint8_t r, uint8_t g, uint8_t b);
		void clear(bool show = true);
		void update();
		void show();
		void show_text(uint8_t x, uint8_t y, String txt);
		void set_brightness(uint8_t val, bool fast = false);
		void cycle_touch_color();

		void set_display_power(bool state);

		// Effects
		uint8_t currentHue = 0;
		void rainbow();
		void rainbowWithGlitter();
		void addGlitter(fract8 chanceOfGlitter);
		void confetti();
		void sinelon();
		void bpm();
		void juggle();
		void lines();
		void fire();
		void show_effect(int index);

		// Icons
		void show_icon(ICON *i, int16_t _x, int16_t _y);
		std::map<const char *, ICON> icons;
		std::vector<ICON *> icons_menu;
		std::vector<ICON *> icons_menu_settings;
		int8_t menu_icons_pos_x_target = 2;
		int8_t menu_icons_pos_x = 2;

		// Balls
		std::vector<BALL> balls;
		void add_ball(uint8_t start_x, uint8_t start_y, int8_t delta_x, int8_t delta_y);

		DeviceOrientation display_orientation = DeviceOrientation::FACE_TOP;

	private:
		FastLED_NeoMatrix *_matrix;

		CRGB leds[144];

		uint32_t CRGBtoUint32(CRGB color);
		uint16_t CRGBtoUint16(CRGB color);
		uint32_t FastColorFade(uint8_t hue, uint8_t fade);
		CRGB HueColorFade(uint8_t hue, uint8_t fade);

		void fill_rainbow_brightness(int numToFill, uint8_t initialhue, uint8_t deltahue, uint8_t val);

		bool fast_brightness_change = false;
};

extern MatrixDisplay display;