
/*
 * This is the main MP game core that games are based from.
 *
 * This class handles basic game states and player lists.
 */

#pragma once

#include "Arduino.h"
#include "utilities/logging.h"
#include <vector>
#include "../audio/audio.h"
#include "../display/display.h"

struct Player
{
	public:
		String name = "";
		uint8_t mac[6];
		uint16_t score = 0;
		bool is_host = false;
};

enum GameState : uint8_t
{
	GAME_SELECT = 0,		// Future : We will start on select a game menu (games will register with core)
	GAME_DEMO = 1,			// Future : Game will run in a Demo mode, any peer change will stop demo, optional
	GAME_NUM_PLAYERS = 2,	// Future : We offer game controlled num players, optional
	GAME_OPTIONS = 3,		// Future : We offer game controlled choices, optional
	GAME_WAITING = 4,		// Waiting for Peers
	GAME_MENU = 5,			//
	GAME_RUNNING = 6,
	GAME_ENDED = 7,
	GAME_RESET = 8
};

static String game_state_names[] =
{
		"SELECT",
		"DEMO",
		"PLAYERS",
		"OPTIONS",
		"WAITING",
		"MENU",
		"RUNNING",
		"ENDED",
		"RESET",
};

class MultiplayerGame
{
	public:
	    MultiplayerGame() {}; // possible have passed back "game ptr"
    	~MultiplayerGame() {};

		void set_num_players(uint8_t num);
		uint8_t get_num_players();
		GameState get_state();
		bool is_hosting();

		// Override optional
		virtual void set_state(GameState s);
		virtual void set_hosting(bool state);
		virtual void peer_added(const uint8_t *mac_addr);
		virtual void peer_removed(const uint8_t *mac_addr);

	    // Virtual functions to be overriden by game class
		virtual bool touched_board(uint8_t x, uint8_t y) = 0;
		virtual bool onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) = 0;
		virtual SFX get_game_wave_file(const char *wav_name) = 0;

    	virtual void display_icon() = 0;
    	virtual void update_loop() = 0;
    	virtual void display_game() = 0;
		virtual void kill_game() = 0;

	private:
		bool is_host = false;
		GameState current_state = GameState::GAME_WAITING;
		uint8_t max_players = 0;
		std::vector<Player> players;
};

extern MultiplayerGame *game;