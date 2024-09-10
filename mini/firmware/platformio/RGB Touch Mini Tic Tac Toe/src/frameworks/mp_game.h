
/*
 * This is the main MP game core that games are based from.
 *
 * This class handles basic game states and player lists.
 */

#pragma once

#include "Arduino.h"
#include "utilities/logging.h"
#include <vector>

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
	GAME_WAITING = 0,
	GAME_MENU = 1,
	GAME_RUNNING = 2,
	GAME_ENDED = 3,
	GAME_RESET = 4
};

static String game_state_names[] =
	{
		"WAITING",
		"MENU",
		"RUNNING",
		"ENDED",
		"RESET",
};

class MultiplayerGame
{
	public:
		void set_num_layers(uint8_t num);
		uint8_t get_num_players();
		GameState get_state();
		virtual void set_state(GameState s);
		bool is_hosting();
		void set_hosting(bool state);

	private:
		bool is_host = false;
		GameState current_state = GameState::GAME_WAITING;
		uint8_t max_players = 0;
		std::vector<Player> players;
};