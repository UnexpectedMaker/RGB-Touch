#include "display.h"
#include "touch/touch.h"
#include "audio/audio.h"
#include "web/wifi_controller.h"

#include "frameworks/game_tictactoe.h"

extern Adafruit_LIS3DH lis;

void MatrixDisplay::begin()
{
	_matrix = new FastLED_NeoMatrix(leds, 12, 12, NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
	FastLED.addLeds<NEOPIXEL, MATRIX_DATA>(leds, 144).setCorrection(TypicalLEDStrip);

	pinMode(MATRIX_PWR, OUTPUT);
	set_display_power(true);

	_matrix->begin();
	_matrix->setTextWrap(false);
	_matrix->setBrightness(0);

	_matrix->setFont(&TomThumb);

	_matrix->setRotation(0);
	_matrix->fillScreen(0);
	_matrix->show();

	icons["um"] = ICON(icon_um_inverse_new, 12, 12);

	icons["cross_small"] = ICON(icon_cross_small, 3, 3);
	icons["circle_small"] = ICON(icon_circle_small, 3, 3);

	icons["plus_small"] = ICON(icon_plus_small, 3, 3);
	icons["arrow_left_small"] = ICON(icon_arrow_left_small, 3, 3);
	icons["arrow_right_small"] = ICON(icon_arrow_right_small, 3, 3);
	icons["arrow_down_small"] = ICON(icon_arrow_down_small, 3, 3);
	icons["arrow_up_small"] = ICON(icon_arrow_up_small, 3, 3);

	// Mode Icons
	icons["mode_settings"] = ICON(icon_menu_settings, 8, 8);
	icons["mode_power"] = ICON(icon_mode_power, 8, 8);
	icons["mode_play"] = ICON(icon_menu_play, 8, 8);

	icons["waiting"] = ICON(icon_waiting_0, 8, 8);
	icons["waiting"].Add_Frame(icon_waiting_1);
	icons["waiting"].Add_Frame(icon_waiting_2);
	icons["waiting"].Add_Frame(icon_waiting_3);
	icons["waiting"].Add_Frame(icon_waiting_4);
	icons["waiting"].Add_Frame(icon_waiting_5);

	// Settings Icons
	icons["menu_settings_volume"] = ICON(icon_settings_volume, 8, 8);
	icons["menu_settings_brightness"] = ICON(icon_settings_brightness_0, 8, 8);

	icons["menu_settings_wifi"] = ICON(icon_settings_wifi_off, 8, 8);
	icons["menu_settings_wifi"].Add_Frame(icon_settings_wifi_man);
	icons["menu_settings_wifi"].Add_Frame(icon_settings_wifi_on);

	icons["menu_settings_rot"] = ICON(icon_settings_rot_lock_off, 8, 8);
	icons["menu_settings_rot"].Add_Frame(icon_settings_rot_lock_on);

	icons_menu.push_back(&icons["mode_power"]);
	icons_menu.push_back(&icons["mode_play"]);
	icons_menu.push_back(&icons["mode_settings"]);

	icons_menu_settings.push_back(&icons["menu_settings_volume"]);
	icons_menu_settings.push_back(&icons["menu_settings_brightness"]);
	icons_menu_settings.push_back(&icons["menu_settings_wifi"]);
	icons_menu_settings.push_back(&icons["menu_settings_rot"]);
}

void MatrixDisplay::set_display_power(bool state)
{
	if (display_powered != state)
	{
		digitalWrite(MATRIX_PWR, (state ? HIGH : LOW));
		display_powered = state;
	}
}

void MatrixDisplay::clear(bool _show)
{
	_matrix->clear();
	if (_show)
		show();
}

void MatrixDisplay::set_brightness(uint8_t val, bool fast)
{
	target_brightness = val;
	fast_brightness_change = fast;
}

void MatrixDisplay::fill_screen(uint8_t r, uint8_t g, uint8_t b)
{
	_matrix->fillScreen(_matrix->Color(r, g, b));
	show();
}

uint32_t MatrixDisplay::CRGBtoUint32(CRGB color)
{
	// Combine the 8-bit R, G, B components into a 32-bit integer
	// Assuming the format is 0xRRGGBB
	return ((uint32_t)color.red << 16) | ((uint32_t)color.green << 8) | (uint32_t)color.blue;
}

uint16_t MatrixDisplay::CRGBtoUint16(CRGB color)
{
	// Scale down the RGB components to fit the 5-6-5 format
	uint8_t red = color.red >> 3;	  // 8-bit to 5-bit
	uint8_t green = color.green >> 2; // 8-bit to 6-bit
	uint8_t blue = color.blue >> 3;	  // 8-bit to 5-bit

	// Combine the components into a single uint16_t
	return ((uint16_t)red << 11) | ((uint16_t)green << 5) | (uint16_t)blue;
}

uint32_t MatrixDisplay::FastColorFade(uint8_t hue, uint8_t fade)
{
	CHSV colHSV(hue, 255, 255);
	CRGB col;
	hsv2rgb_rainbow(colHSV, col);
	return CRGBtoUint32(col.fadeToBlackBy(constrain(fade, 0, 255)));
}

CRGB MatrixDisplay::HueColorFade(uint8_t hue, uint8_t fade)
{
	CHSV colHSV(hue, 255, 255);
	CRGB col;
	hsv2rgb_rainbow(colHSV, col);
	return col.fadeToBlackBy(constrain(fade, 0, 255));
}

void MatrixDisplay::show()
{
	if (display_orientation != rgbtouch.current_orientation)
	{
		// Have it re-orient all of the time for this game
		rgbtouch.requires_re_orientation = true;

		if (rgbtouch.requires_re_orientation || !rgbtouch.started)
		{
			display_orientation = rgbtouch.current_orientation;

			uint8_t new_type = NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE;
			if (display_orientation == DeviceOrientation::FACE_BOTTOM)
				new_type = NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE;
			// else if (display_orientation == DeviceOrientation::FACE_LEFT)
			// 	new_type = NEO_MATRIX_TOP + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE;
			// else if (display_orientation == DeviceOrientation::FACE_RIGHT)
			// 	new_type = NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE;

			_matrix->update_type(new_type);
		}
		else if (display_orientation != DeviceOrientation::FACE_TOP)
		{
			// We always want FACE_TOP if there is no requirement to have the display re-mapped.
			_matrix->update_type(NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE);
			display_orientation = DeviceOrientation::FACE_TOP;
		}
	}

	uint8_t delta = (fast_brightness_change ? 5 : 1);
	bool changed = false;
	if (current_brightness < target_brightness)
	{
		current_brightness = constrain(current_brightness + delta, 0, target_brightness);
		changed = true;
	}
	else if (current_brightness > target_brightness)
	{
		current_brightness = constrain(current_brightness - delta, 0, target_brightness);
		changed = true;
	}

	if (changed)
		_matrix->setBrightness(current_brightness);

	_matrix->show();
}

void MatrixDisplay::update()
{
	if (rgbtouch.check_mode(Modes::FUN))
	{
		if (rgbtouch.check_play_mode(PlayModes::PLAY))
		{
			if (millis() - next_redraw > 25)
			{
				next_redraw = millis();

				game.display_game();

				_matrix->show();
			}
		}
	}
	else
	{
		if (millis() - next_redraw > 50)
		{
			next_redraw = millis();

			_matrix->clear();
			for (size_t b = 0; b < touch.buttons.size(); b++)
			{
				touch.buttons[b].draw_icon(rgbtouch.mode);
			}

			if (menu_icons_pos_x > menu_icons_pos_x_target)
			{
				menu_icons_pos_x--;
				next_redraw += 25;
			}
			else if (menu_icons_pos_x < menu_icons_pos_x_target)
			{
				menu_icons_pos_x++;
				next_redraw += 25;
			}

			if (rgbtouch.check_mode(Modes::MENU))
			{
				for (size_t i = 0; i < icons_menu.size(); i++)
					show_icon(icons_menu[i], menu_icons_pos_x + (12 * i), 0);
			}
			else if (rgbtouch.mode == Modes::SETTINGS)
			{
				for (size_t i = 0; i < icons_menu_settings.size(); i++)
				{
					if (i == (int)SettingsModes::SET_WIFI)
					{
						show_icon(&icons["menu_settings_wifi"], menu_icons_pos_x + (12 * i), 0);
					}
					else
						show_icon(icons_menu_settings[i], menu_icons_pos_x + (12 * i), 0);
				}
			}
			else if (rgbtouch.mode == Modes::SETTINGS_EDIT)
			{
				show_icon(icons_menu_settings[rgbtouch.settings_mode], 0, 0);
				if (rgbtouch.settings_mode == SettingsModes::SET_VOLUME)
				{
					_matrix->fillRect(10, 0, 2, 8, _matrix->Color(0, 0, 0));
					int slider = constrain(((float)rgbtouch.temp_volume / audio_player.max_volume) * 8.0, 0, 8);
					if (slider > 0)
						_matrix->fillRect(10, 8 - slider, 2, slider, _matrix->Color(0, 0, 250));
				}
				else if (rgbtouch.settings_mode == SettingsModes::SET_BRIGHTNESS)
				{
					_matrix->fillRect(10, 0, 2, 8, _matrix->Color(0, 0, 0));
					int slider = constrain(((float)rgbtouch.temp_brightness / max_brightness) * 8.0, 0, 8);
					// info_printf("slider %d\n", slider);
					if (slider > 0)
						_matrix->fillRect(10, 8 - slider, 2, slider, _matrix->Color(0, 0, 250));

					next_redraw += 35;
				}
			}

			show();
		}
	}
}

void MatrixDisplay::fade_leds_to(uint8_t val)
{
	fadeToBlackBy(leds, NUM_LEDS, val);
}
void MatrixDisplay::draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint32_t col)
{
	_matrix->drawLine(x0, y0, x1, y1, col);
}

void MatrixDisplay::draw_circle(uint8_t x, uint8_t y, uint8_t r, uint32_t col)
{
	_matrix->drawCircle(x, y, r, col);
}

int MatrixDisplay::i_sin(int x)
{
	boolean pos = true; // positive - keeps an eye on the sign.
	uint8_t idx;
	// remove next 6 lines for fastestl!
	/*     if (x < 0) {
		   x = -x;
		   pos = !pos;
		 }
		if (x >= 360) x %= 360;   */
	if (x > 180)
	{
		idx = x - 180;
		pos = !pos;
	}
	else
		idx = x;
	if (idx > 90)
		idx = 180 - idx;
	if (pos)
		return isinTable8[idx] / 2;
	return -(isinTable8[idx] / 2);
}

int MatrixDisplay::i_cos(int x)
{
	return i_sin(x + 90);
}

void MatrixDisplay::draw_arc(int16_t x, int16_t y, int16_t r, int16_t rs, int16_t re, uint16_t color)
{
	int8_t _width = 12;
	int8_t _height = 12;

	int16_t l,
		i, w; // int16_t
	int16_t x1, y1, x2, y2;
	unsigned short dw;
	if (re > rs)
		dw = re - rs;
	else
		dw = 256 - rs + re;

	if (dw == 0)
		dw = 256;
	l = (uint8_t)(((((unsigned short)r * dw) >> 7) * (unsigned short)201) >> 8);
	// l = (uint8_t)(((((uint16_t)r * dw) >> 7) * (uint16_t)201)>>7);
	// l = (uint8_t)(((((uint16_t)r * dw) >> 7) * (uint16_t)256)>>7);
	x1 = x + (((int16_t)r * (int16_t)i_cos(rs)) >> 7);
	y1 = y + (((int16_t)r * (int16_t)i_sin(rs)) >> 7);
	for (i = 1; i <= l; i++)
	{
		w = ((unsigned short)dw * (unsigned short)i) / (unsigned short)l + rs;
		// w = ((uint16_t)dw * (uint16_t)i) / (uint16_t)l + rs;
		x2 = x + (((int16_t)r * (int16_t)i_cos(w)) >> 7);
		y2 = y + (((int16_t)r * (int16_t)i_sin(w)) >> 7);
		if ((x1 < _width && x2 < _width) && (y1 < _height && y2 < _height))
			_matrix->drawLine(x1, y1, x2, y2, color);
		x1 = x2;
		y1 = y2;
	}
}

uint32_t MatrixDisplay::Color(uint8_t r, uint8_t g, uint8_t b)
{
	return (_matrix->Color(r, g, b));
}

void MatrixDisplay::show_text(uint8_t x, uint8_t y, String txt, uint32_t col)
{
	_matrix->setTextSize(1);
	_matrix->setCursor(x, y);
	_matrix->setTextColor(col);
	_matrix->print(txt);
	_matrix->show();
}

void MatrixDisplay::show_icon(ICON *i, int16_t _x, int16_t _y, uint8_t v)
{
	uint8_t index = 0;

	for (int16_t y = 0; y < i->height; y++)
	{
		for (int16_t x = 0; x < i->width; x++)
		{
			CHSV col;
			uint16_t val = i->frames[i->current_frame][index++];
			if (val == 0)
				col = CHSV(0, 0, 0);
			else if (val == 1)
				col = CHSV(0, 0, 255);
			else
			{
				val = constrain((uint16_t)((float)val * 0.712), 1, 255);
				col = CHSV((uint8_t)val, 255, v);
			}
			// info_printf("(%d,%d) val:%d\n", x, y, val);
			_matrix->drawPixel((_x + x), (_y + y), col);
		}
	}

	// i->cycle_frame();
}

void MatrixDisplay::cycle_touch_color()
{
	current_touch_color++;
	if (current_touch_color > 255)
		current_touch_color = 0;
}

MatrixDisplay display;