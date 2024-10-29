/***
   RGB Touch Mini - ESP-NOW based Tic Tac Toe
   2024 Unexpected Maker

   This game uses ESP-NOW for device to device communication, so no internet connection
   is required.

   Devices auto poll for ESP-NOW peers, and self determine who is the host.

   For more information about RGB Touch Mini, please visit https://rgbtouch.com

   This is the main code entry point that includes Setup() and Loop() for Arduino.
*/

#include "main.h"

#include <vector>
#include <Wire.h>
#include <driver/rtc_io.h>
#include "touch/touch.h"
#include "audio/audio.h"
#include "settings.h"
#include "display/display.h"
#include "share/share.h"
#include "recorder.h"
#include <LittleFS.h>

#define WAKE_REASON_TOUCH1 BIT64(GPIO_NUM_15)
// #define WAKE_REASON_TOUCH2 BIT64(GPIO_NUM_16)
#define WAKE_REASON_MOVE BIT64(GPIO_NUM_7)

#define VBUS_SENSE 48
#define IMU_INT 7
#define MPR_INT 15

// IMU
Adafruit_LIS3DH lis = Adafruit_LIS3DH();

void setup()
{

	// delay(5000);

	// Was I sleeping or is this a cold boot
	bool was_asleep = rgbtouch.was_sleeping();

	gpio_hold_dis(GPIO_NUM_38);
	gpio_hold_dis(GPIO_NUM_34);
	gpio_hold_dis(GPIO_NUM_15); // MPR_INT
	gpio_hold_dis(GPIO_NUM_7);	// IMU_INT
	gpio_deep_sleep_hold_dis();

	pinMode(MPR_INT, INPUT_PULLUP);
	pinMode(IMU_INT, INPUT_PULLUP);
	pinMode(VBUS_SENSE, INPUT);

	Serial.begin(115200);

	if (!LittleFS.begin(true))
	{
		error_println("LittleFS failed to initialise");
		return;
	}
	else
	{
		delay(100);
		settings.load();
	}

	audio_player.setup(35, 36, 37, 34);

	display.begin();

	if (!touch.initialise_mpr())
	{
		info_println("Failed to initialise MPR121 COL/ROW");
		return;
	}

	if (!lis.begin(0x18))
	{ // change this to 0x19 for alternative i2c address
		info_println("LIS3DH Couldn't start");
	}
	else
	{
		info_println("LIS3DH found!");
		lis.setRange(LIS3DH_RANGE_8_G);
		lis.setINTpolarity(1);
		// lis.setPerformanceMode(LIS3DH_MODE_LOW_POWER);
	}

	delay(100);

	audio_player.set_volume(settings.config.volume);
	display.set_brightness(settings.config.brightness);

	if (!was_asleep)
	{
		// Lets see if we can capture the device orientation to show the logo the right way up on boot
		if (rgbtouch.calc_orientation())
			display.clear(true);

		// We only show the UM logo on a cold boot
		display.show_icon(&display.icons["um"], 0, 0);
		display.show();

		rgbtouch.start_playing = millis();

		audio_player.play_wav("um");
	}
	else
	{
		info_printf("Woke from deep sleep by %d\n", rgbtouch.woke_by());
	}

	Share_ESPNOW::getInstance().init_esp_now();
}

void loop()
{
	// Seems we always need audio
	audio_player.update();

	if (rgbtouch.shutting_down)
	{
		display.set_brightness(0, false);
		display.show();

		if (millis() - rgbtouch.shutdown_timer > 3000)
		{
			pinMode(21, OUTPUT);
			digitalWrite(21, HIGH);
		}

		return;
	}

	/*
	 * This is all for the fancy logo and mode display at the start
	 */

	if (millis() - rgbtouch.start_playing < 1500)
	{
		display.show();
		return;
	}
	else if (!rgbtouch.show_mode)
	{
		rgbtouch.show_mode = true;
		info_println("Ready!");
		display.clear();

		rgbtouch.change_mode(Modes::FUN);
		rgbtouch.change_play_mode(PlayModes::PLAY);
		display.show_icon(display.icons_menu[(int)rgbtouch.play_mode], 2, 2);
		display.show();
		audio_player.play_wav_queue(audio_player.play_mode_voices[(int)rgbtouch.play_mode]);

		rgbtouch.start_playing = millis();
		return;
	}
	else if (!rgbtouch.started)
	{
		rgbtouch.started = true;
		// recorder.rec = Recording(display.fade_step);
		display.clear();
		return;
	}

	/*
	 * End of fancy logo and mode display at the start
	 */

	// calc device orientation and if device moved
	if (rgbtouch.calc_orientation())
	{
		// If the device has been moved, we reset last touch like it has been touched.
		// This way we dont need to track individual timers
		touch.last_touch = millis();

		// info_println("Was Moved");
	}

	if (touch.process_touches(display.current_touch_color))
	{
		if (rgbtouch.check_play_mode(PlayModes::PLAY))
			display.cycle_touch_color();
	}

	// We dont want to mess with the brightness and display power while we are editing the brightness settings
	if (!rgbtouch.check_mode(Modes::SETTINGS_EDIT) || rgbtouch.settings_mode != SettingsModes::SET_BRIGHTNESS)
	{
		// Allow game to decide we sleep
		if ( game && game->control_sleep() ) {

		} 
		else if (millis() - touch.last_touch > rgbtouch.get_sleep_timer(1))
		{
			// We need to shut down nicely here and go into deep sleep
			display.set_brightness(0, false);
			display.set_display_power(false);
			if (!rgbtouch.vbus_preset())
				rgbtouch.go_to_sleep();
			else
				touch.last_touch = millis();
		}
		else if (millis() - touch.last_touch > rgbtouch.get_sleep_timer(0))
		{
			// We need to dim the screen now as it's not been touched for 10 seconds
			display.set_brightness(settings.config.brightness / 2, false);
		}
		else
		{
			display.set_brightness(settings.config.brightness, true);
			display.set_display_power(true);
		}
	}

	if ( game ) {
		game->update_loop();
	}

	display.update();

	wifi_controller.loop();

	// this only gets called internally every 10 seconds
	Share_ESPNOW::getInstance().find_peers();

	// if (touch.wifi_started && millis() - touch.wifi_start_delay > 1000)
	recorder.recieve_recording();
}

void RGBTouch::shutdown()
{
	display.set_brightness(0);
	audio_player.play_wav_queue("goodbye");
	shutdown_timer = millis();
	shutting_down = true;
}

uint32_t RGBTouch::get_sleep_timer(uint8_t stage)
{
	if (vbus_preset())
		return (settings.config.deep_sleep_dimmer_USB[stage]);
	else
		return (settings.config.deep_sleep_dimmer_BAT[stage]);
}

bool RGBTouch::was_sleeping() { return (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1); }

int RGBTouch::woke_by()
{
	if (esp_sleep_get_ext1_wakeup_status() == WAKE_REASON_TOUCH1)
		return 1;
	else if (esp_sleep_get_ext1_wakeup_status() == WAKE_REASON_MOVE)
		return 2;
	else
		return 0;
}

void RGBTouch::go_to_sleep()
{
	info_println("Going to sleep");
	// if the web server is running, we don't want to go to sleep...
	// if (web_server.is_running())
	// {
	// 	info_println("Web server is running, exiting call to sleep");
	// 	return;
	// }

	// if (wifi_controller.is_connected())
	// 	wifi_controller.disconnect(true);

	// if (wifi_controller.is_busy())
	// {
	// 	info_println("Cant sleep yet, wifi is busy!");
	// 	return;
	// }

	// // Dont call this if the task was not created!
	// wifi_controller.kill_controller_task();

	// settings.save(true);
	// delay(100);
	LittleFS.end();

	digitalWrite(38, LOW);
	pinMode(34, OUTPUT);
	digitalWrite(34, LOW);

	gpio_hold_en(GPIO_NUM_7);
	gpio_hold_en(GPIO_NUM_15);
	gpio_hold_en(GPIO_NUM_38);
	gpio_hold_en(GPIO_NUM_34);
	gpio_deep_sleep_hold_en();

	esp_sleep_enable_ext1_wakeup(WAKE_REASON_TOUCH1, ESP_EXT1_WAKEUP_ANY_LOW);
	esp_deep_sleep_start();
}

bool RGBTouch::calc_orientation()
{
	// If it's not time to read the input, return early
	if (next_imu_read > millis())
		return false;

	// Set the next read time to 200ms from now
	next_imu_read = millis() + 200;

	sensors_event_t event;
	lis.getEvent(&event);

	double x_Buff = 0;
	double y_Buff = 0;
	double z_Buff = 0;

	uint8_t samples = 0;

	while (++samples < 5)
	{
		x_Buff += float(event.acceleration.x);
		y_Buff += float(event.acceleration.y);
		z_Buff += float(event.acceleration.z);
	}

	x_Buff = (x_Buff / samples - 1);
	y_Buff = (y_Buff / samples - 1);
	z_Buff = (z_Buff / samples - 1);

	orientation_roll = atan2(y_Buff, z_Buff) * 57.3;
	orientation_pitch = atan2((-x_Buff), sqrt(y_Buff * y_Buff + z_Buff * z_Buff)) * 57.3;

	double move_delta = abs(last_x_accel - x_Buff) + abs(last_y_accel - y_Buff) + abs(last_z_accel - z_Buff);
	bool moved = false;

	if (move_delta > 2.5)
	{
		moved = true;
		last_move = millis();
	}

	// info_printf("x_Buff %f, y_Buff %f, z_Buff %f, move_delta %f\n", x_Buff, y_Buff, z_Buff, move_delta);

	last_x_accel = x_Buff;
	last_y_accel = y_Buff;
	last_z_accel = z_Buff;

	// info_printf("roll %f, pitch %f\n", orientation_roll, orientation_pitch);

	const int8_t DEADZONE = 35;

	if (abs(orientation_pitch) > abs(orientation_roll))
	{
		if (orientation_pitch < -DEADZONE)
			current_orientation = DeviceOrientation::FACE_TOP;
		else if (orientation_pitch > DEADZONE)
			current_orientation = DeviceOrientation::FACE_BOTTOM;
	}
	// else
	// {
	// 	if (orientation_roll < -DEADZONE)
	// 		current_orientation = DeviceOrientation::FACE_RIGHT;
	// 	else if (orientation_roll > DEADZONE)
	// 		current_orientation = DeviceOrientation::FACE_LEFT;
	// }

	// info_printf("Orientation: %s\n", orientation_names[(int)current_orientation]);

	return moved;
}

bool RGBTouch::vbus_preset()
{
	return (digitalRead(VBUS_SENSE));
}

/*
NAVIGATION AND CALLBACKS
*/

bool RGBTouch::check_play_mode(PlayModes new_mode)
{
	return (play_mode == new_mode);
}

void RGBTouch::change_play_mode(PlayModes new_mode)
{
	rgbtouch.play_mode = new_mode;
	info_printf("New Play Mode: %d\n", rgbtouch.play_mode);

	// Anything to process after we change mode
	if (rgbtouch.check_play_mode(PlayModes::PLAY))
		display.fade_step = 10;

	set_swipes();

	// Only one mode, so no need to save the selection
	// settings.save(true);
}

bool RGBTouch::check_mode(Modes new_mode)
{
	return (mode == new_mode);
}

void RGBTouch::change_mode(Modes new_mode)
{
	// // Anything to process before we change mode
	// if (touch.mode == Modes::FUN)
	display.clear(false);

	rgbtouch.mode = new_mode;
	info_printf("New mode: %d\n", rgbtouch.mode);

	// Dont save the current mode when it's just the menu
	// if (mode != Modes::MENU)
	// {
	// 	settings.config.last_mode = (int)mode;
	// 	settings.save(true);
	// }
}

bool RGBTouch::start_recording()
{
	if (rgbtouch.check_play_mode(PlayModes::PLAY) && (wifi_controller.is_connected() || share_espnow.getInstance().has_peer()))
	{
		audio_player.play_wav("start");
		touch.bump_next_touch(2000);
		recorder.rec.start_time = millis();
		recorder.is_recording = true;
		info_println("Recording started...");
		touch.clear_all_touches();
		recorder.rec.touch_recording.clear();
		recorder.rec.touch_recording.shrink_to_fit();

		info_printf("touch count?: %d\n", touch.touches.size());

		return true;
	}
	else
	{
		audio_player.play_menu_beep(0, 0);
	}

	return false;
}

bool RGBTouch::switch_to_menu()
{
	display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.play_mode);
	display.menu_icons_pos_x = display.menu_icons_pos_x_target;
	rgbtouch.menu_mode = (MenuModes)rgbtouch.play_mode;
	rgbtouch.change_mode(Modes::MENU);
	audio_player.play_menu_beep(11, 0);
	audio_player.play_wav_queue("menu");

	rgbtouch.temp_volume = settings.config.volume;
	rgbtouch.temp_brightness = settings.config.brightness;
	rgbtouch.temp_orientation = settings.config.orientation;

	// We clear the display early with show to allow the device re-orientation to kick in before showing the menu.
	// prevents a flash of it wrong before the change if the device was re-oriented since last time.
	display.clear(true);

	return true;
}

bool RGBTouch::menu_cancel()
{
	if (rgbtouch.check_mode(Modes::MENU))
	{
		rgbtouch.change_mode(Modes::FUN);
		display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.menu_mode);
		display.menu_icons_pos_x = display.menu_icons_pos_x_target;
		audio_player.play_menu_beep(11, 0);
		audio_player.play_wav_queue(audio_player.play_mode_voices[(int)rgbtouch.play_mode]);
		display.display_orientation = DeviceOrientation::UNSET;
		display.clear(true);
		return true;
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS))
	{
		rgbtouch.change_mode(Modes::MENU);
		display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.menu_mode);
		display.menu_icons_pos_x = display.menu_icons_pos_x_target;
		audio_player.play_menu_beep(11, 0);
		return true;
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS_EDIT))
	{
		if (rgbtouch.settings_mode == SettingsModes::SET_VOLUME)
		{
			rgbtouch.temp_volume = settings.config.volume;
			audio_player.set_volume(rgbtouch.temp_volume);
		}
		else if (rgbtouch.settings_mode == SettingsModes::SET_BRIGHTNESS)
		{
			// Set the brightness back to the saved value in settings
			display.set_brightness(settings.config.brightness, true);
		}

		rgbtouch.change_mode(Modes::SETTINGS);
		display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.settings_mode);
		display.menu_icons_pos_x = display.menu_icons_pos_x_target;
		audio_player.play_menu_beep(11, 0);
		audio_player.play_wav_queue("cancel");
		return true;
	}

	return false;
}

void RGBTouch::set_swipes()
{
	touch.allow_swipes(false, false);
}

bool RGBTouch::menu_accept()
{
	// audio_player.play_wav("ok");
	if (rgbtouch.check_mode(Modes::MENU))
	{
		if (rgbtouch.menu_mode == MenuModes::MENU_SETTINGS)
		{
			rgbtouch.change_mode(Modes::SETTINGS);
			display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.settings_mode);
			display.menu_icons_pos_x = display.menu_icons_pos_x_target;
			audio_player.play_menu_beep(11, 0);
			audio_player.play_wav_queue("settings");
			return true;
		}
		else if (rgbtouch.menu_mode == MenuModes::MENU_POWER)
		{
			display.set_brightness(0, false);
			audio_player.play_menu_beep(11, 0);
			audio_player.play_wav_queue("power_off");
			rgbtouch.shutdown();
			return true;
		}
		else
		{
			rgbtouch.change_play_mode((PlayModes)rgbtouch.menu_mode);
			rgbtouch.change_mode(Modes::FUN);
			audio_player.play_menu_beep(11, 0);
			audio_player.play_wav_queue(audio_player.play_mode_voices[(int)rgbtouch.menu_mode]);
			display.display_orientation = DeviceOrientation::UNSET;
			display.clear(true);

			return true;
		}
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS))
	{
		if (rgbtouch.settings_mode != SettingsModes::SET_WIFI)
		{
			rgbtouch.change_mode(Modes::SETTINGS_EDIT);
			display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.settings_mode);
			display.menu_icons_pos_x = display.menu_icons_pos_x_target;
			audio_player.play_menu_beep(11, 0);
			audio_player.play_wav_queue(audio_player.settings_mode_voices[(int)rgbtouch.settings_mode]);
			return true;
		}
		else
		{
			info_println("SET_WIFI ACCEPT");
			audio_player.play_menu_beep(11, 0);
			audio_player.play_wav_queue((rgbtouch.wifi_started ? "stopping" : "starting"));

			rgbtouch.wifi_start_delay = millis();
			rgbtouch.wifi_started = !rgbtouch.wifi_started;
			if (!rgbtouch.wifi_started && wifi_controller.is_connected())
				wifi_controller.disconnect(true);

			return true;
		}
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS_EDIT))
	{
		bool saved = false;
		if (rgbtouch.settings_mode == SettingsModes::SET_VOLUME)
		{
			settings.config.volume = rgbtouch.temp_volume;
			settings.save(true);
			saved = true;
		}
		else if (rgbtouch.settings_mode == SettingsModes::SET_BRIGHTNESS)
		{
			// Update the brightness value in settings with teh current target value
			settings.config.brightness = rgbtouch.temp_brightness;
			display.set_brightness(settings.config.brightness, true);
			settings.save(true);
			saved = true;
		}

		rgbtouch.change_mode(Modes::SETTINGS);
		display.menu_icons_pos_x_target = 2 - (12 * rgbtouch.settings_mode);
		display.menu_icons_pos_x = display.menu_icons_pos_x_target;
		audio_player.play_menu_beep(11, 0);
		if (saved)
			audio_player.play_wav_queue("saved");
		else
			audio_player.play_wav_queue("ok");

		return true;
	}

	return false;
}

bool RGBTouch::menu_left()
{
	if (rgbtouch.check_mode(Modes::MENU))
	{
		if ((int)rgbtouch.menu_mode > 0)
		{
			rgbtouch.menu_mode = (MenuModes)(rgbtouch.menu_mode - 1);
			display.menu_icons_pos_x_target += 12;
			audio_player.play_menu_beep(11, 0);
			return true;
		}
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS))
	{
		if ((int)rgbtouch.settings_mode > 0)
		{
			rgbtouch.settings_mode = (SettingsModes)(rgbtouch.settings_mode - 1);
			display.menu_icons_pos_x = display.menu_icons_pos_x_target;
			display.menu_icons_pos_x_target += 12;
			audio_player.play_menu_beep(11, 0);
			return true;
		}
	}

	return false;
}

bool RGBTouch::menu_right()
{
	if (rgbtouch.check_mode(Modes::MENU))
	{
		if ((int)rgbtouch.menu_mode < display.icons_menu.size() - 1)
		{
			rgbtouch.menu_mode = (MenuModes)(rgbtouch.menu_mode + 1);
			display.menu_icons_pos_x_target -= 12;
			audio_player.play_menu_beep(11, 0);
			return true;
		}
	}
	else if (rgbtouch.check_mode(Modes::SETTINGS))
	{
		if ((int)rgbtouch.settings_mode < display.icons_menu_settings.size() - 1)
		{
			rgbtouch.settings_mode = (SettingsModes)(rgbtouch.settings_mode + 1);
			display.menu_icons_pos_x_target -= 12;
			audio_player.play_menu_beep(11, 0);
			return true;
		}
	}

	return false;
}

bool RGBTouch::menu_down()
{
	if (rgbtouch.check_mode(Modes::SETTINGS_EDIT))
	{
		if (rgbtouch.settings_mode == SettingsModes::SET_VOLUME)
		{
			if (rgbtouch.temp_volume > 0)
			{
				rgbtouch.temp_volume = constrain(rgbtouch.temp_volume - 5, 0, audio_player.max_volume);
				audio_player.set_volume(rgbtouch.temp_volume);
				audio_player.play_menu_beep(11, 0);
				return true;
			}
		}
		else if (rgbtouch.settings_mode == SettingsModes::SET_BRIGHTNESS)
		{
			if (rgbtouch.temp_brightness > display.min_brightness)
			{
				rgbtouch.temp_brightness = constrain(rgbtouch.temp_brightness - 15, display.min_brightness, display.max_brightness);
				display.set_brightness(rgbtouch.temp_brightness, true);
				audio_player.play_menu_beep(11, 0);
				return true;
			}
		}
	}

	return false;
}

bool RGBTouch::menu_up()
{
	if (rgbtouch.check_mode(Modes::SETTINGS_EDIT))
	{
		if (rgbtouch.settings_mode == SettingsModes::SET_VOLUME)
		{
			if (rgbtouch.temp_volume < audio_player.max_volume)
			{
				rgbtouch.temp_volume = constrain(rgbtouch.temp_volume + 5, 0, audio_player.max_volume);
				audio_player.set_volume(rgbtouch.temp_volume);
				audio_player.play_menu_beep(11, 0);
				return true;
			}
		}
		else if (rgbtouch.settings_mode == SettingsModes::SET_BRIGHTNESS)
		{
			if (rgbtouch.temp_brightness < display.max_brightness)
			{
				rgbtouch.temp_brightness = constrain(rgbtouch.temp_brightness + 15, display.min_brightness, display.max_brightness);
				display.set_brightness(rgbtouch.temp_brightness, true);
				audio_player.play_menu_beep(11, 0);
				return true;
			}
		}
	}

	return false;
}

RGBTouch rgbtouch;