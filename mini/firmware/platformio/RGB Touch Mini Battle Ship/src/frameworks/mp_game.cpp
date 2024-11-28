/*
 * This is the main MP game core that games are based from.
 *
 * This class handles basic game states and player lists.
 */

#include "mp_game.h"

void MultiplayerGame::set_num_players(uint8_t num)
{
	max_players = num;
}

uint8_t MultiplayerGame::get_num_players()
{
	return players.size();
}

GameState MultiplayerGame::get_state()
{
	return current_state;
}

void MultiplayerGame::set_state(GameState s)
{
	current_state = s;
	info_printf("Game State: %s\n", game_state_names[(uint8_t)current_state]);
}

bool MultiplayerGame::is_hosting()
{
	return is_host;
}

void MultiplayerGame::set_hosting(bool state)
{
	is_host = state;
	info_printf("Am host? %s\n", (is_host ? "YES" : "NO"));

	set_state(GameState::GAME_MENU);
}

void MultiplayerGame::peer_added(const uint8_t *mac_addr)
{

}

void MultiplayerGame::peer_removed(const uint8_t *mac_addr)
{

}

bool MultiplayerGame::control_sleep()
{
	return false;
}


// Base class pointer , extended class will have override functionality
MultiplayerGame *game;