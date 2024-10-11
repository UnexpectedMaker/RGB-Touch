
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
	GAME_NUM_PLAYERS = 1,	// Future : We offer game controlled num players, optional
	GAME_OPTIONS = 2,		// Future : We offer game controlled choices, optional
	GAME_WAITING = 3,		// Waiting for Peers (if required)
	GAME_MENU = 4,			//
	GAME_RUNNING = 5,
	GAME_ENDED = 6,
	GAME_RESET = 7
};

static String game_state_names[] =
	{
		"SELECT",
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

		void set_num_layers(uint8_t num);
		uint8_t get_num_players();
		GameState get_state();
		virtual void set_state(GameState s);
		bool is_hosting();
		void set_hosting(bool state);

	    // Virtual functions to be overriden by game class
		virtual void start_game(uint8_t piece) = 0;
		virtual bool touched_board(uint8_t x, uint8_t y) = 0;
		virtual void update_position(uint8_t a, uint8_t b) = 0;
		virtual void set_piece(uint8_t piece) = 0;
		virtual bool onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) = 0;

    	virtual void display_game() = 0;
		virtual void end_game() = 0;
		virtual void reset_game() = 0;
		virtual SFX get_game_wave_file(const char *wav_name) = 0;

	private:
		bool is_host = false;
		GameState current_state = GameState::GAME_WAITING;
		uint8_t max_players = 0;
		std::vector<Player> players;
};

extern MultiplayerGame *game;