#pragma once

#include "json.h"
#include "json_conversions.h"
#include "utilities/logging.h"
#include <vector>
#include <map>

using json = nlohmann::json;

struct wifi_station
{
		String ssid = "";
		String pass = "";
		uint8_t channel = 9;
};

// Save data struct
struct Config
{
		bool wifi_start = false;
		bool wifi_check_for_updates = true;

		std::vector<wifi_station> wifi_options;
		uint8_t current_wifi_station = 0;

		String mdns_name = "rgbtouch";
		bool website_darkmode = true;

		// UTC and Country settings - country needed for some widgets like Open Weather
		String country = "";
		String city = "";
		int16_t utc_offset = 999;

		uint8_t orientation = 0;
		uint8_t brightness = 20;
		uint8_t volume = 60;
		int8_t rotation_lock = 0; // 0 not locked, -1 bottom, 1 top

		uint8_t last_mode = 1; // last mode selected

		// Array: value 1 is time before screen dim, and value 2 is time before deep sleep
		// Second value is wanted duration + initial duration, so 60 seconds before deep sleep is 60 + initial 30 for dimming
		std::vector<uint32_t> deep_sleep_dimmer_USB = {30000, 90000};
		std::vector<uint32_t> deep_sleep_dimmer_BAT = {15000, 45000};

		json last_saved_data;
};

class Settings
{

	public:
		Config config;
		// std::vector<setting_group> settings_groups;

		Settings()
		{
		}

		bool load();
		bool save(bool force);
		bool backup();
		bool create();
		void print_file();
		bool has_wifi_creds(void);
		bool has_country_set(void);
		void update_wifi_credentials(String ssid, String pass);

		bool ui_forced_save = false; //

	private:
		static constexpr const char *filename = "/settings.json";
		static constexpr const char *tmp_filename = "/tmp_settings.json";
		static constexpr const char *log = "/log.txt";
		static constexpr const char *backup_prefix = "settings_back_";
		static const int max_backups = 10;
		static long backupNumber(const String);

		unsigned long max_time_between_saves = 30000; // every 30 seconds
		unsigned long last_save_time = 0;
};

extern Settings settings;
