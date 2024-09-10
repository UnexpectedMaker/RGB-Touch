#pragma once

#include <map>
#include <Adafruit_GFX.h> // Library Manager
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

static const uint8_t isinTable8[] = {
	// clang-format off
  0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 44, 
  49, 53, 57, 62, 66, 70, 75, 79, 83, 87, 
  91, 96, 100, 104, 108, 112, 116, 120, 124, 128, 
  131, 135, 139, 143, 146, 150, 153, 157, 160, 164, 
  167, 171, 174, 177, 180, 183, 186, 190, 192, 195, 
  198, 201, 204, 206, 209, 211, 214, 216, 219, 221, 
  223, 225, 227, 229, 231, 233, 235, 236, 238, 240, 
  241, 243, 244, 245, 246, 247, 248, 249, 250, 251, 
  252, 253, 253, 254, 254, 254, 255, 255, 255, 255,
	// clang-format on
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
		uint8_t max_brightness = 150;
		bool display_powered = false;

		void begin();

		void clear(bool show = true);
		void update();
		void show();

		void set_brightness(uint8_t val, bool fast = false);
		uint8_t do_PATTERNS();
		void cycle_touch_color();

		void set_display_power(bool state);

		// Drawing routines
		void fade_leds_to(uint8_t val);
		void fill_screen(uint8_t r, uint8_t g, uint8_t b);
		void draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint32_t col);
		void draw_circle(uint8_t x, uint8_t y, uint8_t r, uint32_t col);
		void draw_arc(int16_t x, int16_t y, int16_t r, int16_t rs, int16_t re, uint16_t color);
		uint32_t Color(uint8_t r, uint8_t g, uint8_t b);
		void show_text(uint8_t x, uint8_t y, String txt, uint32_t col);

		int i_sin(int x);
		int i_cos(int x);

		// Icons
		void show_icon(ICON *i, int16_t _x, int16_t _y, uint8_t v = 255);
		std::map<const char *, ICON> icons;
		std::vector<ICON *> icons_menu;
		std::vector<ICON *> icons_menu_settings;
		int8_t menu_icons_pos_x_target = 2;
		int8_t menu_icons_pos_x = 2;

		DeviceOrientation display_orientation = DeviceOrientation::FACE_TOP;

	private:
		FastLED_NeoMatrix *_matrix;

		CRGB leds[144];

		uint32_t CRGBtoUint32(CRGB color);
		uint16_t CRGBtoUint16(CRGB color);
		uint32_t FastColorFade(uint8_t hue, uint8_t fade);
		CRGB HueColorFade(uint8_t hue, uint8_t fade);

		bool fast_brightness_change = false;
};

extern MatrixDisplay display;