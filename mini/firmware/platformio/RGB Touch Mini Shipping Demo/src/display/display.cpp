#include "display.h"
#include "touch/touch.h"
#include "audio/audio.h"
#include "web/wifi_controller.h"

extern Adafruit_LIS3DH lis;

// Magic Sand Stuff
Adafruit_PixelDust magic_sand(12, 12, N_GRAINS, 1, 64, false);

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

	icons["cross"] = ICON(icon_cross, 5, 5);
	icons["cross_small"] = ICON(icon_cross_small, 3, 3);
	icons["plus_small"] = ICON(icon_plus_small, 3, 3);
	icons["arrow_left_small"] = ICON(icon_arrow_left_small, 3, 3);
	icons["arrow_right_small"] = ICON(icon_arrow_right_small, 3, 3);
	icons["arrow_down_small"] = ICON(icon_arrow_down_small, 3, 3);
	icons["arrow_up_small"] = ICON(icon_arrow_up_small, 3, 3);

	// Mode Icons
	icons["mode_settings"] = ICON(icon_menu_settings, 8, 8);
	icons["mode_power"] = ICON(icon_mode_power, 8, 8);
	icons["mode_play"] = ICON(icon_menu_play, 8, 8);
	icons["mode_beeps"] = ICON(icon_mode_beeps, 8, 8);
	icons["mode_sand"] = ICON(icon_mode_sand, 8, 8);
	icons["mode_piano"] = ICON(icon_mode_piano, 8, 8);
	icons["mode_bounce"] = ICON(icon_mode_bounce, 8, 8);
	icons["mode_patterns"] = ICON(icon_mode_patterns, 8, 8);

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
	icons_menu.push_back(&icons["mode_beeps"]);
	icons_menu.push_back(&icons["mode_sand"]);
	icons_menu.push_back(&icons["mode_piano"]);
	icons_menu.push_back(&icons["mode_bounce"]);
	icons_menu.push_back(&icons["mode_patterns"]);
	icons_menu.push_back(&icons["mode_settings"]);

	icons_menu_settings.push_back(&icons["menu_settings_volume"]);
	icons_menu_settings.push_back(&icons["menu_settings_brightness"]);
	icons_menu_settings.push_back(&icons["menu_settings_wifi"]);
	icons_menu_settings.push_back(&icons["menu_settings_rot"]);

	lis.setRange(LIS3DH_RANGE_4_G); // 2, 4, 8 or 16 G!

	if (!magic_sand.begin())
	{
		delay(5000);
		info_println("Couldn't start sand");
	}

	magic_sand.randomize(_matrix->Color(117, 7, 135), true);
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
		rgbtouch.requires_re_orientation = (!rgbtouch.check_mode(Modes::FUN) || rgbtouch.check_play_mode(PlayModes::PIANO) || rgbtouch.check_play_mode(PlayModes::PATTERNS));
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
	// We are either booting, or we are in FUN or MENU modes.
	if (rgbtouch.check_mode(Modes::FUN))
	{
		if (rgbtouch.check_play_mode(PlayModes::PLAY) || rgbtouch.check_play_mode(PlayModes::BEEPS))
		{
			if (millis() - next_redraw > 50)
			{
				next_redraw = millis();
				_matrix->clear();

				for (int t = 0; t < touch.touches.size(); t++)
				{
					if (touch.touches[t].fade_amount < 255)
					{
						_matrix->drawPixel(touch.touches[t].x, touch.touches[t].y, FastColorFade(touch.touches[t].color, touch.touches[t].fade_amount));
						touch.touches[t].FadeStep(display.fade_step);
					}
				}
				show();
			}
		}
		else if (rgbtouch.check_play_mode(PlayModes::PATTERNS))
		{
			if (millis() - next_redraw > next_redraw_step)
			{
				next_redraw = millis();

				show_effect(settings.config.current_effect);
				show();
			}
		}
		else if (rgbtouch.check_play_mode(PlayModes::MAGIC_SAND))
		{
			if (millis() - next_redraw > 15)
			{
				next_redraw = millis();

				bool was_touched = false;

				if (touch.touches.size() > 0)
				{
					was_touched = true;
					magic_sand.attract(touch.touches[0].x, touch.touches[0].y, _matrix->Color(0, 0, 255));

					for (size_t i = 0; i < touch.touches.size(); i++)
						touch.touches[i].FadeStep(display.fade_step);
				}
				else
				{
					sensors_event_t event;
					lis.getEvent(&event);

					double xx, yy, zz;
					xx = event.acceleration.y * 250;
					yy = event.acceleration.x * 250;
					zz = event.acceleration.z * 250;

					// Run one frame of the simulation
					magic_sand.iterate(xx, yy, zz);
				}

				// Update pixel data in LED driver
				dimension_t x, y;
				grain_col_t grain_col;
				_matrix->fillScreen(0x0);
				for (int i = 0; i < N_GRAINS; i++)
				{
					magic_sand.getPosition(i, &x, &y, &grain_col);
					_matrix->drawPixel(x, y, grain_col);
				}

				show();
			}
		}
		else if (rgbtouch.check_play_mode(PlayModes::PIANO))
		{
			if (millis() - next_redraw > 50)
			{
				_matrix->clear();
				next_redraw = millis();
				// white keys top row
				_matrix->drawLine(11, 3, 11, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(9, 3, 9, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(7, 3, 7, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(5, 3, 5, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(4, 3, 4, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(2, 3, 2, 5, _matrix->Color(200, 200, 200));
				_matrix->drawLine(0, 3, 0, 5, _matrix->Color(200, 200, 200));

				// white keys bottom row
				_matrix->drawLine(11, 9, 11, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(9, 9, 9, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(7, 9, 7, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(5, 9, 5, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(4, 9, 4, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(2, 9, 2, 11, _matrix->Color(200, 200, 200));
				_matrix->drawLine(0, 9, 0, 11, _matrix->Color(200, 200, 200));

				// black keys top row
				_matrix->drawLine(10, 0, 10, 1, _matrix->Color(50, 50, 255));
				_matrix->drawLine(8, 0, 8, 1, _matrix->Color(50, 50, 255));
				_matrix->drawLine(6, 0, 6, 1, _matrix->Color(50, 50, 255));
				_matrix->drawLine(3, 0, 3, 1, _matrix->Color(50, 50, 255));
				_matrix->drawLine(1, 0, 1, 1, _matrix->Color(50, 50, 255));

				// black keys bottom row
				_matrix->drawLine(10, 7, 10, 8, _matrix->Color(50, 50, 255));
				_matrix->drawLine(8, 7, 8, 8, _matrix->Color(50, 50, 255));
				_matrix->drawLine(6, 7, 6, 8, _matrix->Color(50, 50, 255));
				_matrix->drawLine(3, 7, 3, 8, _matrix->Color(50, 50, 255));
				_matrix->drawLine(1, 7, 1, 8, _matrix->Color(50, 50, 255));

				show();

				for (int t = 0; t < touch.touches.size(); t++)
				{
					if (touch.touches[t].fade_amount < 255)
					{
						touch.touches[t].FadeStep(display.fade_step);
					}
				}
			}
		}
		else if (rgbtouch.check_play_mode(PlayModes::BOUNCE))
		{
			if (millis() - next_redraw > 150)
			{
				if (!balls.empty())
				{
					next_redraw = millis();
					_matrix->clear();

					for (int b = 0; b < balls.size(); b++)
					{
						if (balls[b].active)
						{
							if (balls[b].move())
								audio_player.play_note(0, 0, balls[b].vol_fade, 0.2);

							_matrix->drawPixel(balls[b]._x[0], balls[b]._y[0], HueColorFade(balls[b].col, balls[b].fade));
							uint8_t col = 200;
							for (int t = 1; t <= balls[b].trail_length; t++)
							{
								_matrix->drawPixel(balls[b]._x[t], balls[b]._y[t], CRGB(col, col, col).fadeToBlackBy(balls[b].fade));
								col -= 50;
							}
						}
						else
						{
							balls.erase(balls.begin() + b);
						}
					}
					_matrix->show();
				}
			}
		}
	}
	// MENU mode stuff here
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
						// if (wifi_controller.is_connected())
						// 	show_icon(&icons["menu_settings_wifi_on"], menu_icons_pos_x + (12 * i), 0);
						// else
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

					if (slider > 0)
						_matrix->fillRect(10, 8 - slider, 2, slider, _matrix->Color(0, 0, 250));

					next_redraw += 35;
				}
			}

			show();
		}
	}
}

void MatrixDisplay::show_text(uint8_t x, uint8_t y, String txt)
{
	_matrix->setTextSize(1);
	_matrix->setCursor(x, y);
	_matrix->setTextColor(_matrix->Color(0, 0, 255));
	_matrix->print(txt);
	_matrix->show();

	info_println(txt);

	delay(2000);
}

void MatrixDisplay::show_icon(ICON *i, int16_t _x, int16_t _y)
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
				col = CHSV((uint8_t)val, 255, 255);
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
void MatrixDisplay::add_ball(uint8_t start_x, uint8_t start_y, int8_t delta_x, int8_t delta_y)
{
	balls.push_back(BALL(start_x, start_y, constrain(delta_x, -3, 3), constrain(delta_y, -3, 3), random(255)));
}

MatrixDisplay display;