#include "touch.h"
#include "audio/audio.h"

#include "display/display.h"

// You can have up to 4 on one i2c bus but one is enough for testing!
Adafruit_MPR121 cap_col = Adafruit_MPR121();
Adafruit_MPR121 cap_row = Adafruit_MPR121();

bool TouchMatrix::initialise_mpr()
{
	// Init MPR121 for ROWS
	if (!cap_row.begin(0x5B))
	{
		info_println("MPR121 for ROWS not found, check wiring?");
		display.fill_screen(100, 0, 0);
		return false;
	}

	delay(100);

	// Init MPR121 for COLS
	if (!cap_col.begin(0x5A))
	{
		info_println("MPR121 for COLUMNS not found, check wiring?");
		display.fill_screen(100, 0, 0);
		return false;
	}
	info_println("MPR121 COLUMNS & ROWS found!");

	//  cap_col.writeRegister(MPR121_CONFIG1, 0x10);
	//  cap_col.writeRegister(MPR121_CONFIG2, 0x22);
	//
	//  cap_row.writeRegister(MPR121_CONFIG1, 0x1-);
	//  cap_row.writeRegister(MPR121_CONFIG2, 0x22);
	//
	//  cap_row.writeRegister(MPR121_CONFIG1, 0x20);
	//  cap_row.writeRegister(MPR121_CONFIG2, 0x3A);

	// cap_col.setThresholds(2, 0);
	// cap_row.setThresholds(2, 0);

	cap_col.setThresholds(1, 0);
	cap_row.setThresholds(1, 0);

	setup_buttons();

	return true;
}

void TouchMatrix::setup_buttons()
{
	TouchButton record = TouchButton(0, 0, 4, 4, 700, rgbtouch.start_recording, Modes::FUN);
	buttons.push_back(record);

	TouchButton menu = TouchButton(4, 4, 4, 4, 700, rgbtouch.switch_to_menu, Modes::FUN);
	buttons.push_back(menu);

	TouchButton menu_cancel = TouchButton(6, 9, 3, 3, 250, rgbtouch.menu_cancel, Modes::MENU);
	menu_cancel.add_extra_mode(Modes::SETTINGS);
	menu_cancel.add_extra_mode(Modes::SETTINGS_EDIT);
	menu_cancel.set_icon(&display.icons["cross_small"]);
	buttons.push_back(menu_cancel);

	TouchButton menu_plus = TouchButton(3, 9, 3, 3, 30, rgbtouch.menu_accept, Modes::MENU);
	menu_plus.add_extra_mode(Modes::SETTINGS);
	menu_plus.add_extra_mode(Modes::SETTINGS_EDIT);
	menu_plus.set_icon(&display.icons["plus_small"]);
	buttons.push_back(menu_plus);

	TouchButton menu_arrow_left = TouchButton(0, 9, 3, 3, 30, rgbtouch.menu_left, Modes::MENU);
	menu_arrow_left.add_extra_mode(Modes::SETTINGS);
	menu_arrow_left.set_icon(&display.icons["arrow_left_small"]);
	buttons.push_back(menu_arrow_left);

	TouchButton menu_arrow_right = TouchButton(9, 9, 3, 3, 30, rgbtouch.menu_right, Modes::MENU);
	menu_arrow_right.add_extra_mode(Modes::SETTINGS);
	menu_arrow_right.set_icon(&display.icons["arrow_right_small"]);
	buttons.push_back(menu_arrow_right);

	TouchButton menu_arrow_down = TouchButton(0, 9, 3, 3, 30, rgbtouch.menu_down, Modes::SETTINGS_EDIT);
	menu_arrow_down.set_icon(&display.icons["arrow_down_small"]);
	buttons.push_back(menu_arrow_down);

	TouchButton menu_arrow_up = TouchButton(9, 9, 3, 3, 30, rgbtouch.menu_up, Modes::SETTINGS_EDIT);
	menu_arrow_up.set_icon(&display.icons["arrow_up_small"]);
	buttons.push_back(menu_arrow_up);
}

void TouchMatrix::block_touch_input(bool block)
{
	block_touch = block;
}

void TouchMatrix::allow_swipes(bool horiz, bool vert)
{
	can_swipe_h = horiz;
	can_swipe_v = vert;
}

bool TouchMatrix::can_swipe()
{
	return (can_swipe_h || can_swipe_h);
}

bool TouchMatrix::process_touches(uint8_t col)
{
	if (!rgbtouch.check_mode(Modes::FUN))
	{
		if (millis() - next_touch < 50)
			return false;
	}
	else if (!recorder.is_playing && !recorder.is_recording)
	{
		if (millis() - next_touch < 10)
			return false;
	}
	else if (recorder.is_recording)
	{
		if (millis() - next_touch < 25)
			return false;
	}

	next_touch = millis();
	// Clean up dead touches first
	// A dead touch is one that has faded out

	if (!rgbtouch.check_mode(Modes::FUN))
	{
		clear_all_touches();
	}
	else
	{
		bool found_dead = false;
		for (int t = 0; t < touches.size(); t++)
		{
			if (touches[t].is_dead())
			{
				touches.erase(touches.begin() + t);
				found_dead = true;
			}
		}
	}

	// This is where we play back a recording no matter where it came from :)
	// This is called every loop
	if (recorder.is_playing)
	{
		if (recorder.rec.touch_recording.size() == 0)
		{
			recorder.is_playing = false;
			return false;
		}

		if (millis() >= recorder.rec.playback_start_time + recorder.rec.touch_recording[recorder.rec.playback_index].created)
		{
			RecordedPoint &rp = recorder.rec.touch_recording[recorder.rec.playback_index];
			// info_printf("point %d: millis() %d - wanted %d, waiting %d\n", i, millis(), (start_playback + rp.created), (start_playback + rp.created) - millis());

			Point p = Point(rp.x, rp.y, rp.color);

			if (rp.touched)
			{
				// if this was a re touched point, we dont want to add a new one,
				// but instead we want to reset the fade of the current one
				for (int t = 0; t < touches.size(); t++)
				{
					if (p.same(touches[t]))
					{
						touches[t].Retouch(rp.color);
						break;
					}
				}
			}
			else
			{
				touches.push_back(p);
			}
			recorder.rec.playback_index++;

			if (recorder.rec.playback_index == recorder.rec.touch_recording.size())
			{
				recorder.rec.touch_recording.clear();
				recorder.rec.playback_index = 0;
			}
		}

		return false;
	}

	int touch_count = 0;

	// Capture the touch matrix state
	currtouched_cols = cap_col.touched();
	currtouched_rows = cap_row.touched();

	for (int _i = 0; _i < MATRIX_SIZE; ++_i)
	{
		int last_i = -1;
		int last_j = -1;

		if (readColumn(_i)) // Matrix X axis
		{
			for (int _j = 0; _j < MATRIX_SIZE; ++_j)
			{
				if (readRow(_j)) // Matrix Y axis
				{
					touch_count++;

					int j = _j;
					int i = _i;

					// Why could there be touches in the list when we are not touching? Because we fade the LEDs out
					// and keep the touch there until it's fully faded.

					// We now need to compensate for the device orientation - boo hoo
					// But only if we need to

					if (rgbtouch.requires_re_orientation)
					{
						if (rgbtouch.current_orientation == DeviceOrientation::FACE_BOTTOM)
						{
							i = constrain(11 - i, 0, 11);
							j = constrain(11 - j, 0, 11);
						}
					}

					// info_printf("touched (%d, %d) - %d\n", i, j, touches.size());
					Point p = Point(i, j, col);

					// If swiping it enabled, record swipe data
					if (can_swipe() || rgbtouch.check_play_mode(PlayModes::BOUNCE))
					{
						if (swipe_touches.size() < 2)
							swipe_touches.push_back(p);
						else
						{
							swipe_touches[1].x = i;
							swipe_touches[1].y = j;
						}
					}

					// If there are no touches in the list, then just add this new one.
					if (touches.size() == 0 || (last_i != i && last_j != j))
					{
						touches.push_back(p);

						if (rgbtouch.check_mode(Modes::FUN))
						{
							// track a new recorded point as well
							if (recorder.is_recording)
								recorder.rec.touch_recording.push_back(RecordedPoint(i, j, col, false, millis() - recorder.rec.start_time));

							if (rgbtouch.check_play_mode(PlayModes::BEEPS))
							{
								if (rgbtouch.current_orientation == DeviceOrientation::FACE_TOP)
								{
									i = constrain(11 - i, 0, 11);
									j = constrain(11 - j, 0, 11);
								}
								audio_player.play_tone((17 - i) * 55, (1.0 + (float)j / 12.0));
							}
							else if (rgbtouch.check_play_mode(PlayModes::PIANO))
								audio_player.play_note(i, j);
						}
					}
					else
					{
						// We cant to check if a touch at this position is already there.
						// If so, we dont add a new one, but "re-touch" the existing one to extend it's life
						bool found = false;
						for (int t = 0; t < touches.size(); t++)
						{
							if (p.same(touches[t]))
							{
								found = true;
								if (rgbtouch.check_play_mode(PlayModes::PATTERNS))
									break;

								touches[t].Retouch(col);
								found = true;

								// track a re-touched recorded point as well
								if (recorder.is_recording)
									recorder.rec.touch_recording.push_back(RecordedPoint(i, j, col, true, millis() - recorder.rec.start_time));

								break;
							}
						}

						// If we didn't find an existing point to re-touch, we add a new one
						if (!found)
						{
							touches.push_back(p);

							if (rgbtouch.check_mode(Modes::FUN))
							{
								// track a recorded point as well
								if (recorder.is_recording)
									recorder.rec.touch_recording.push_back(RecordedPoint(i, j, col, false, millis() - recorder.rec.start_time));

								if (rgbtouch.check_play_mode(PlayModes::BEEPS))
								{
									if (rgbtouch.current_orientation == DeviceOrientation::FACE_TOP)
									{
										i = constrain(11 - i, 0, 11);
										j = constrain(11 - j, 0, 11);
									}
									audio_player.play_tone((17 - i) * 55, (1.0 + (float)j / 12.0));
								}
								else if (rgbtouch.check_play_mode(PlayModes::PIANO))
									audio_player.play_note(i, j);
							}
						}
					}
					last_i = i;
					last_j = j;
				}
			}
		}
	}

	// We have touches this pass, so let's process them
	if (touch_count > 0)
	{
		// Track the last time we touched for recording purposes
		last_touch = millis();

		// Update swipe data if swiping is enabled
		if (can_swipe() || rgbtouch.check_play_mode(PlayModes::BOUNCE))
		{
			if (touch_start_begin)
			{
				touch_last_x = swipe_touches[1].x;
				touch_last_y = swipe_touches[1].y;
			}
			else
			{
				touch_start_x = swipe_touches[0].x;
				touch_start_y = swipe_touches[0].y;
				touch_start_begin = true;
				touch_start_delay = millis();
			}
		}

		// Process any buttons that might have been pressed first
		for (size_t t = 0; t < touches.size(); t++)
		{
			Point &tch = touches[t];
			for (size_t b = 0; b < buttons.size(); b++)
			{
				if (buttons[b].check_bounds(tch.x, tch.y))
				{
					if (buttons[b].press())
					{
						touch_count = 0;
						clear_all_touches();
					}
				}
			}
		}
	}
	else
	{
		// We didn't track any touches this pass, so let's see if we ended a swipe
		if ((can_swipe() || rgbtouch.check_play_mode(PlayModes::BOUNCE)) && (touch_start_begin && millis() - touch_start_delay > 300))
		{
			touch_start_delay = millis();
			touch_start_begin = false;

			int8_t delta_x = touch_last_x - touch_start_x;
			int8_t delta_y = touch_last_y - touch_start_y;

			if (rgbtouch.check_play_mode(PlayModes::BOUNCE))
			{
				if (abs(delta_x) > 1 || abs(delta_y) > 1)
				{
					display.add_ball(touch_last_x, touch_last_y, delta_x, delta_y);
					audio_player.play_note(6, 6, 1.0, 0.3);
				}
			}
			else
			{
				if (abs(delta_x) > 3 || abs(delta_y) > 3)
				{
					if (can_swipe_h && abs(delta_x) > abs(delta_y))
					{
						if (delta_x < 0)
						{
							info_println("swipe right");
							if (rgbtouch.check_play_mode(PlayModes::PATTERNS))
							{
								settings.config.current_effect++;
								if (settings.config.current_effect == 6)
									settings.config.current_effect = 0;

								settings.save(true);
							}
						}
						else
						{
							if (rgbtouch.check_play_mode(PlayModes::PATTERNS))
							{
								if (settings.config.current_effect == 0)
									settings.config.current_effect = 5;
								else
									settings.config.current_effect--;

								settings.save(true);
							}
							info_println("swipe left");
						}
					}
					else if (can_swipe_v)
					{
						if (delta_y < 0)
						{
							// audio_player.play_wav("down");
							info_println("swipe down");
						}
						else
						{
							// audio_player.play_wav("up");
							info_println("swipe up");
						}
					}
				}
			}

			swipe_touches.clear();
		}

		// If there are no touches, then no buttons can be getting pressed!
		// So we reset all of he button states
		for (size_t b = 0; b < buttons.size(); b++)
			buttons[b].reset();

		// If we are recording, check if there has been a 3 second pause between last touch, which ends the recording
		// We now also track the max recording length - if time since recording_start_time is more than 20 seconds, we end the recording
		if (recorder.is_recording && (millis() - last_touch > 3000 || millis() - recorder.rec.start_time > recorder.max_recording_time))
		{
			recorder.is_recording = false;
			info_printf("Recording finished after %d seconds with %d recorded points!\n\n", (millis() - recorder.rec.start_time), recorder.rec.touch_recording.size());
			recorder.rec.playback_start_time = millis();
			recorder.rec.playback_index = 0;

			if (!recorder.share_recording())
				audio_player.play_wav("error");
			else
				audio_player.play_note(11, 11);
		}
	}

	return (touch_count > 0);
}

void TouchMatrix::bump_next_touch(unsigned long amount)
{
	next_touch += amount;
}

void TouchMatrix::clear_all_touches()
{
	touches.clear();
	touches.shrink_to_fit();
}

bool TouchMatrix::readCell(int x, int y)
{
	if (USE_SENSITIVITY_BUFFER)
		return (currtouched_cols & (1 << x) && default_Filter[0][x] - cap_col.filteredData(x) > SENSITIVITY_BUFFER) &&
			   (currtouched_rows & (1 << y) && default_Filter[1][y] - cap_row.filteredData(y) > SENSITIVITY_BUFFER);
	else
		return (currtouched_cols & (1 << x)) && (currtouched_rows & (1 << y));
}

bool TouchMatrix::readColumn(int x)
{
	// info_printf("X %d: %d - %d\n", x, cap_col.filteredData(x), (default_Filter[0][x] - cap_col.filteredData(x)));
	if (USE_SENSITIVITY_BUFFER)
		return (currtouched_cols & (1 << x) && default_Filter[0][x] - cap_col.filteredData(x) > SENSITIVITY_BUFFER);
	else
		return (currtouched_cols & (1 << x));
}

bool TouchMatrix::readRow(int y)
{
	// info_printf("Y %d: %d - %d\n", y, cap_col.filteredData(y), (default_Filter[1][y] - cap_row.filteredData(y)));
	if (USE_SENSITIVITY_BUFFER)
		return (currtouched_rows & (1 << y) && default_Filter[1][y] - cap_row.filteredData(y) > SENSITIVITY_BUFFER);
	else
		return (currtouched_rows & (1 << y));
}

void TouchMatrix::create_ripple(uint8_t x, uint8_t y, uint8_t c, bool round, TouchButton *button)
{
	Ripple r;
	r.x = x;
	r.y = y;
	r.a = 0;
	r.r = 0;
	r.c = c;
	r.round = round;
	PATTERNS.push_back(r);
}

SWIPE TouchMatrix::did_swipe()
{
	return SWIPE::NA;
}

TouchMatrix touch;
