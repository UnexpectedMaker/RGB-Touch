/***
   RGB Touch Mini - ESP-NOW based Tic Tac Toe
   2024 Unexpected Maker

   This game uses ESP-NOW for device to device communication, so no internet connection
   is required.

   Devices auto poll for ESP-NOW peers, and self determine who is the host.

   For more information about RGB Touch Mini, please visit https://rgbtouch.com

   This is the main code entry point that includes Setup() and Loop() for Arduino.
*/

#pragma once

#include "Arduino.h"
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

enum DeviceOrientation
{
	FACE_TOP = 0,
	FACE_RIGHT = 1,
	FACE_BOTTOM = 2,
	FACE_LEFT = 3,
	UNSET = 4,
};

enum Modes
{
	FUN = 0,
	MENU = 1,
	SETTINGS = 2,
	SETTINGS_EDIT = 3,
};

enum PlayModes
{
	PLAY = 1,
};

enum MenuModes
{
	MENU_POWER = 0,
	MENU_PLAY = 1,
	MENU_SETTINGS = 2,
};

enum SettingsModes
{
	SET_VOLUME = 0,
	SET_BRIGHTNESS = 1,
	SET_WIFI = 2,
	SET_ORIENTATION = 3,
};

static SettingsModes settings_modes[] = {
	SettingsModes::SET_VOLUME, SettingsModes::SET_BRIGHTNESS, SettingsModes::SET_WIFI, SettingsModes::SET_ORIENTATION
};

class RGBTouch
{
	public:
		const String version_firmware = "v1.0";
		const String version_year = "2024";

		bool vbus_preset();

		void change_mode(Modes new_mode);
		bool check_mode(Modes new_mode);

		bool check_play_mode(PlayModes new_mode);
		void change_play_mode(PlayModes new_mode);
		void set_swipes();

		static bool switch_to_menu();
		static bool start_recording();

		void shutdown();
		uint32_t get_sleep_timer(uint8_t stage);

		static bool menu_cancel();
		static bool menu_accept();
		static bool menu_left();
		static bool menu_right();
		static bool menu_up();
		static bool menu_down();

		// Deep Sleep stuff
		void go_to_sleep();
		bool was_sleeping();
		int woke_by();

		// IMU
		bool calc_orientation();

		Modes mode = Modes::FUN;
		PlayModes play_mode = PlayModes::PLAY;
		MenuModes menu_mode = MenuModes::MENU_PLAY;
		SettingsModes settings_mode = SettingsModes::SET_VOLUME;

		DeviceOrientation current_orientation = DeviceOrientation::FACE_TOP;
		bool requires_re_orientation = false;

		uint8_t temp_volume;
		uint8_t temp_brightness;
		uint8_t temp_orientation;

		unsigned long start_playing = 0;
		bool started = false;
		bool show_mode = false;
		bool wifi_started = false;
		unsigned long wifi_start_delay = 0;

		bool shutting_down = false;
		unsigned long shutdown_timer = 0;

	private:
		unsigned long next_imu_read = 0;
		double orientation_roll = 0.00;
		double orientation_pitch = 0.00;
		double last_x_accel = 0;
		double last_y_accel = 0;
		double last_z_accel = 0;
		unsigned long last_move = 0;
};

extern RGBTouch rgbtouch;