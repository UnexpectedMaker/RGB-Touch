/*
 * A multi-player Battle Ship game
 *
 * Uses ESP-NOW for device to device communication

audio_player.play_note(6, 11, 1.0, 1);
audio_player.play_note(11, 11, 1.0, 1);
audio_player.play_note(0, 0, 1.0, 1);


 */

#include <algorithm>
#include <vector>

#include "../../display/display.h"
#include "../../touch/touch.h"
#include "../../share/share.h"

// Game specfic files
#include "game_battleship.h"

#include "audio/voice/gameover.h"
#include "audio/voice/battleship.h"

BattleShip battleShip;	// Create instance , so we can notify core


BattleShip::BattleShip()
{
	// TODO : Core to have multi game options
	game = this;	// Base class to take init
	
	// ADD SFX used by game
	game_wav_files = {
//		{"miss", SFX(voice_miss, sizeof(voice_miss))},
//		{"hit", SFX(voice_hit, sizeof(voice_hit))},
		{"gameover", SFX(gameover, sizeof(gameover))},
		{"battleship", SFX(gameover, sizeof(gameover))},
	};

	// Seed rand with deviceID , this should produce differed rands on different devices
    uint64_t deviceMac = 0LL;
    esp_efuse_mac_get_default((uint8_t*) (&deviceMac));
	std::srand( (unsigned int) deviceMac );
}

bool BattleShip::touched_board(uint8_t x, uint8_t y)
{
	lastTouch.clear();
//	info_printf("touched_board count?: %d\n", touch.touches.size());

	 if ( bs_state == BattleShipState::BS_ENDING )
		return false;

	// hold the last point for when we have no touches
	Point p =  Point(x, y, 0);
	lastTouch.clear();
	lastTouch.push_back(p);
	lastPressedTime = millis();

	if (get_state() == GameState::GAME_MENU)
	{
//		info_printf("Game State: %s\n", game_state_names[(uint8_t)get_state()]);

		if ((host_starts && is_hosting()) || (!host_starts && !is_hosting()))
		{
			return true;
		}
	} 
	else if (get_state() == GameState::GAME_RUNNING)
	{
		if ( my_turn ) {

			return true;
		}
	} 

	return false;
}

// Change state , logic and clear
void BattleShip::change_game_state(BattleShipState s)
{
	bs_state = s;
	info_printf("Change Game Battle State: %s\n", battleship_state_names[(uint8_t)bs_state]);

	switch( bs_state ) {

		// TODO : have a demo mode ?

		//
		case BattleShipState::BS_BOARD_SETUP:
		{
			// Make function
			{
				players_ready = 0;

				ai_level = ( std::rand() % 3 ) + 1;

				// Clear game data before we start a battle
				ships.clear();
				shots_fired.clear();
				incoming_shots.clear();
				ships_kiiled.clear();

				info_printf("create_random_board AI[%d] Board\n", ai_level);

				battleShip.create_random_board();
			}

			// AI Tester - move to Class 
			{
				tempLastms = millis();

				// Build possible shots
				available_shots.clear();
				for ( u_int y=0; y < MATRIX_SIZE; y++) {
					for ( u_int x=0; x < MATRIX_SIZE; x++) {

						Dot d = Dot(x, y, 0);
						available_shots.push_back(d);
					}
				}

				// Lets Randomize the possible shots list - Make this based on AI Level
				for (u_int l = 0; l < ai_level; l++) {
					for (u_int i = 0; i < available_shots.size()-1; i++) {
						int target = i + 1 + rand() % (available_shots.size() - i - 1);
						std::swap( available_shots[i], available_shots[target] );
					}
				}
			}

			// choose demo
			if (share_espnow.getInstance().has_peer())
			{
				set_state(GameState::GAME_MENU);
				bs_state = BattleShipState::BS_BOARD_READY;

				// Setup temp to be our counter before we lock the board
				tempLastms = millis();
				tempCounter = MATRIX_SIZE;
			}
			else 
			{
				// Demo go straight to BS Active 
				bs_state = BattleShipState::BS_ACTIVE;
			}
		}
		break;

		case BattleShipState::BS_AWAITING_ENEMY:
			set_state(GameState::GAME_RUNNING);

			send_data(BS_DataType::WAITING_TO_PLAY,0,0,0,0);
		break;

		case BattleShipState::BS_ENDING:
		{
			audio_player.play_wav_queue("gameover");

			tempLastms = millis();
			tempCounter = 10;
		}
		break;

	}
}

void BattleShip::process_last_touch(uint8_t x, uint8_t y)
{
	if (get_state() == GameState::GAME_MENU)
	{
		info_printf("process_last_touch Game State: %s\n", game_state_names[(uint8_t)get_state()]);

		if ((host_starts && is_hosting()) || (!host_starts && !is_hosting()))
		{
			switch(bs_state) {

				// reset if touched while in board ready 
				case BattleShipState::BS_BOARD_READY:
				{
					change_game_state( BattleShipState::BS_BOARD_SETUP );
				}
				break;
			}

/*
//					player_piece = BoardPiece::CROSS;
//					send_data(DataType::SET_PIECE, (uint8_t)BoardPiece::CIRCLE, 0);
enum BS_DataType : uint8_t
{
	WANT_TO_PLAY = 0,		// FUIURE to force them into this game mode
	WAITING_TO_PLAY = 1, 

	// TODO : more User is building board
	// * allow user to rotate pieces
	// ** Tap on piece rotates
	// * allow user to move pieces 
	// ** Select first then select area to move top or left most section

	SEND_MOVE = 3, // More user fire at square
	SEND_DESTROYED_SHIP = 4,

	// End of game winner sends ships not destroyed
	SEND_ALIVE_SHIP = 5,

	END_GAME = 6,
};				}
				else
				{
//					player_piece = BoardPiece::CIRCLE;
//					send_data(DataType::SET_PIECE, (uint8_t)BoardPiece::CROSS, 0);
				}

				my_turn = true;

				set_state(GameState::GAME_RUNNING);
			}
			*/
		}
	} else if (get_state() == GameState::GAME_RUNNING) {

		if ( bs_state == BattleShipState::BS_ACTIVE ) {

			info_printf("GAME TOUCH ACTIVE x: %d, y: %d - %s\n", x, y , battleship_state_names[(uint8_t)bs_state]);

			if ( my_turn ) {


				// TODO : check if have fired here before 
				// * Send fire and return
			}

			// TODO : play error
		}
	}
}
/*
bool BattleShip::set_position(uint8_t position, uint8_t piece)
{
	if (!my_turn)
	{
		audio_player.play_note(0, 0, 1, 0.2);
		return false;
	}

	if (position < 9)
	{
		if (board[position] == (uint8_t)BoardPiece::EMPTY)
		{
			board[position] = piece;
			pos_fader[position] = 255;

			moves_left--;

			winning_player = check_winner();

			if (winning_player > 0)
			{
				send_data(DataType::END_GAME, position, piece);
			}
			else
			{
				send_data(DataType::SEND_MOVE, position, piece);
			}

			my_turn = false;

			return true;
		}
	}

	audio_player.play_note(0, 0, 1, 0.2);
	return false;
}
*/

/*
void BattleShip::update_position(uint8_t position, uint8_t piece)
{
	if (position < 9)
	{
		if (board[position] == (uint8_t)BoardPiece::EMPTY)
		{
			board[position] = (uint8_t)piece;
			pos_fader[position] = 0;

			moves_left--;

			winning_player = check_winner();

			if (winning_player == 0)
				my_turn = true;
		}
	}
}
*/



void BattleShip::update_loop()
{
	// Lets make random cycle every loop
	std::rand();

	// Process last touch (ie on lift off)
	if ( touch.touches.size() <= 0 && lastTouch.size() > 0 )
	{
		// Debounce .. we should move this into touch to get an up / down function
		if ( millis() - lastPressedTime >= DEBONCE ) 
		{
			battleShip.process_last_touch(lastTouch[0].x, lastTouch[0].y);
			lastTouch.clear();
		}
	}

	if (!share_espnow.getInstance().has_peer()) {
//		info_printf("GAME STATE NO PEERS: %s\n", game_state_names[(uint8_t)get_state()]);
	} else {
//		info_printf("GAME STATE PEERS: %s\n", game_state_names[(uint8_t)get_state()]);
	}

	// Start a demo mode for now
	if (get_state() == GameState::GAME_WAITING)
	{
		// Toggle between demo and waiting mode
		if (share_espnow.getInstance().has_peer()) {
			kill_game();
			return;
		}

		// If we have no setup , lets create a board
		if ( bs_state == BattleShipState::BS_NONE ) {
			change_game_state( BattleShipState::BS_BOARD_SETUP );		
		}
		
		// Waiting is Demo Mode for now
		if ( bs_state == BattleShipState::BS_ACTIVE ) {

			// TODO : move this to its own AI class

			// 1/2 a Second fire rate
			if ( millis() - tempLastms >= 500 ) {
				tempLastms = millis();

				// TODO : improve by 
				// * Knowing which direction caused a chain hit and carry in that direction
				// * Using ships destroyed to know whats left, thus only looking where those ships can fit 
				Dot shot = available_shots.back();
//				info_printf("Next Shot x: %d, y: %d\n", shot.x, shot.y );
				// Do not look back if we are empty or last hit was a kill
				if ( shot.color != KILL_COLOR && !shots_fired.empty() ) 
				{
					// Cycle last "X" to detect if one was a hit with open neighours
					int aiLevel = (AI_LOOK_BACK*ai_level);
					int lookBackTo = constrain( (int) shots_fired.size()-aiLevel, 0, shots_fired.size() - 1);

//					info_printf("Look back : %d from %d\n", lookBackTo , shots_fired.size()-1 );

					for( u_int checkShot = shots_fired.size()-1; checkShot >= lookBackTo;  checkShot-- ) 
					{
						Dot oldShot = shots_fired[checkShot];

						// If no hit, next option
						if ( oldShot.color == SHOT_COLOR ) {
//							info_printf("Shot Past[%d] x: %d, y: %d\n",checkShot, oldShot.x, oldShot.y );
							continue;
						}

						// if was a kill, keep it to the pulled
						if ( oldShot.color == KILL_COLOR ) {
//							info_printf("Kill Past[%d] x: %d, y: %d\n",checkShot, oldShot.x, oldShot.y );
							break;
						}

						std::vector<int> connectedIndex;
//						info_printf("Find Neighbours[%d] x: %d, y: %d\n",checkShot , oldShot.x, oldShot.y );

						// Build a list of connected options still available
						for (auto i = 0; i < available_shots.size(); i++)
						{
							// TODO : only allow matches in a direction we discovered 

							if ( available_shots[i].y == oldShot.y ) 
							{
								if (available_shots[i].x == oldShot.x-1) {
									connectedIndex.push_back(i);
//									info_printf("Neightbour[%d] x: %d, y: %d\n",i , oldShot.x-1, oldShot.y );
								}
								if (available_shots[i].x == oldShot.x+1) {
									connectedIndex.push_back(i);
//									info_printf("Neightbour[%d] x: %d, y: %d\n",i , oldShot.x+1, oldShot.y );
								}
							}
							
							if ( available_shots[i].x == oldShot.x ) 
							{
								if (available_shots[i].y == oldShot.y-1) {
									connectedIndex.push_back(i);
//									info_printf("Neightbour[%d] x: %d, y: %d\n",i , oldShot.x, oldShot.y-1 );
								}
								if (available_shots[i].y == oldShot.y+1){
									connectedIndex.push_back(i);
//									info_printf("Neightbour[%d] x: %d, y: %d\n",i , oldShot.x, oldShot.y+1 );
								}
							}
						}

						// If no connectors next look back
						if ( connectedIndex.size() <= 0 ) {
//							info_printf("No Neightbour\n" );
							continue;
						}

						// Pick an available connector
						// TODO : remember which way we went
						int index = 0;

						if ( connectedIndex.size() > 1 ) {
							index = std::rand() % connectedIndex.size();
						}

						// move choice near a neightbour to last elemebt
						index = connectedIndex[index];
						std::swap( available_shots[index], available_shots[available_shots.size() - 1] );

						shot = available_shots.back();
//						info_printf("Shoot Neightbour x: %d, y: %d\n" , shot.x, shot.y );
						break;
					}
				}

				// Drop last one , either original or latest swap
				available_shots.pop_back();

				std::string sfx = "beep";

				uint16_t fireColor = SHOT_COLOR;	

				for (SHIP &vessel : ships) {
					int8_t hitsLeft = vessel.hitCheck(shot.x, shot.y,false);

					// Future to test kill vessel if count = 0
					if ( hitsLeft >= 0 ) {
						if ( hitsLeft == 0 ) {
							fireColor = KILL_COLOR;
							sfx = "destroy";
						} else {
							fireColor = HIT_COLOR;
							sfx = "hit";
						}

						// Force vessel into destoryed
						if ( hitsLeft == 0 ) {
							ships_kiiled.push_back( vessel.duplicate(true) );
							audio_player.play_note(6, 11, 1.0, 1);
						} else {
							audio_player.play_note(0, 6, 1.0, 0.2);
						}
					}
				}

				//
				// audio_player.play_wav_queue(sfx);

				// Set fired in demo
				Dot d = Dot(shot.x, shot.y, fireColor);
				shots_fired.push_back(d);
//				info_printf("Shoot x: %d, y: %d\n" , shot.x, shot.y );
			}
		}
	} else if (get_state() == GameState::GAME_MENU) {

		// in menu reset board
		if ( bs_state != BattleShipState::BS_BOARD_READY ) {
			kill_game();
			change_game_state( BattleShipState::BS_NONE );		
		}


	} else if (get_state() == GameState::GAME_RUNNING) {



	} 
	
	{
		if (!share_espnow.getInstance().has_peer()) {
//			info_printf("GAME STATE NO PEERS: %s\n", game_state_names[(uint8_t)get_state()]);
		}

		// 
		if ( bs_state == BattleShipState::BS_NONE ) {
			change_game_state( BattleShipState::BS_BOARD_SETUP );		
		}
	}

	switch (bs_state) {

		case BattleShipState::BS_BOARD_SETUP:
		break;

		case BattleShipState::BS_BOARD_READY:
			
			// Count X every second till we 
			if ( millis() - tempLastms >= 1000 ) 
			{
				tempLastms = millis();

				tempCounter--;

				if ( tempCounter == 0 ) {
					info_printf("Board applied \n");
					change_game_state( BattleShipState::BS_AWAITING_ENEMY );
				}
			}
		break;

		case BattleShipState::BS_AWAITING_ENEMY:
			// TODO : waiting on enemy to be ready 
		break;

		case BattleShipState::BS_ACTIVE:
		{
			// Check END 
			battleShip.check_end_game();

			// TODO : logic on player ? 

			break;
		}

		case BattleShipState::BS_ENDING:
		{
			// TODO : some form of animation
			if ( millis() - tempLastms >= 1000 ) 
			{
				tempLastms = millis();

				tempCounter--;

				if ( tempCounter == 0 ) {
					info_printf("Game Over to new board\n");
					change_game_state( BattleShipState::BS_NONE );
				}
			}
		}
		break;
	}
}

void BattleShip::display_game()
{

	if (get_state() == GameState::GAME_WAITING) {
		// DEMO MODE - move to logic 

		// If we have no setup , lets create a board - move to logic
		if ( bs_state == BattleShipState::BS_NONE ) {
			display.clear();
			change_game_state( BattleShipState::BS_BOARD_SETUP );
		}

		//
		render_game_board(true);

/*		
		// TODO ? Change ?
		if (millis() - next_display_update > wait_period)
		{
			next_display_update = millis();

			display.fade_leds_to(80);
			if (!share_espnow.getInstance().has_peer())
			{
				display.draw_line(waiting_radius[0], 0, waiting_radius[0], 11, display.Color(0, 0, 128));
				waiting_radius[0] += 1;
				if (waiting_radius[0] == 12)
					waiting_radius[0] = 0;

				wait_period = 100;
			}
			else
			{
				uint32_t col = display.Color(0, 128, 0);
				if (is_hosting())
					col = display.Color(0, 0, 128);

				display.draw_line(6 + waiting_radius[0], 0, 6 + waiting_radius[0], 11, col);
				display.draw_line(5 - waiting_radius[0], 0, 5 - waiting_radius[0], 11, col);
				waiting_radius[0] += 1;
				if (waiting_radius[0] >= 6)
					waiting_radius[0] = 0;

				wait_period = 150;
			}
		}
*/		
	}
	else if (get_state() == GameState::GAME_MENU)
	{
		// 
		render_game_board(false);

		if ( bs_state == BattleShipState::BS_BOARD_READY ) {
			display.draw_line(0,MATRIX_SIZE-1,tempCounter-1,MATRIX_SIZE-1, display.Color(128, 0, 0) );
		}

	}
	else if (get_state() == GameState::GAME_RUNNING)
	{
		render_game_board(false);
		
/*
		for (uint8_t i = 0; i < 9; i++)
		{
			bool found = false;
			if (board[i] == BoardPiece::CROSS)
			{
				found = true;
				display.show_icon(&display.icons["cross_small"], piece_pos_x[i], piece_pos_y[i], pos_fader[i]);
			}
			else if (board[i] == BoardPiece::CIRCLE)
			{
				found = true;
				display.show_icon(&display.icons["circle_small"], piece_pos_x[i], piece_pos_y[i], pos_fader[i]);
			}
			if (found && winning_player == 0)
				pos_fader[i] = constrain(pos_fader[i] + 10, 0, 255);
		}

		// flash winning pieces, but not if it's a draw
		if (winning_player > 0 && winning_player < 99)
		{
			for (size_t i = 0; i < 3; i++)
			{
				uint8_t _flash = pos_fader[winning_combinations[winning_combo][i]];
				if (i == 0)
				{
					if ((_flash == 255 && flashing_winner) || (_flash == 50 && !flashing_winner))
					{
						flashing_winner = !flashing_winner;
						winner_flash_counter--;
						if (winner_flash_counter == 0)
						{
							end_game();
						}
					}
				}

				pos_fader[winning_combinations[winning_combo][i]] = constrain(_flash + (flashing_winner ? 10 : -10), 50, 255);
				// info_printf("winner flash %d %d %d\n", winning_combo, i, pos_fader[winning_combinations[winning_combo][i]]);
			}
		}
		else if (winning_player == 99)
		{
			winner_flash_counter--;
			if (winner_flash_counter == 0)
			{
				end_game();
			}
		}
*/
	}
	else if (get_state() == GameState::GAME_ENDED)
	{
		info_printf("GAME_ENDED State: %s\n", game_state_names[(uint8_t)get_state()]);

		display.clear();
	}
}

void BattleShip::render_game_board(bool demoMode)
{
	display.clear();

	switch (bs_state) {

		case BattleShipState::BS_BOARD_SETUP:
		case BattleShipState::BS_BOARD_READY:
		case BattleShipState::BS_AWAITING_ENEMY:
		{
			// Draw ships always
			for (SHIP &vessel : ships) {
				vessel.draw();
			}
		}
		break;

		case BattleShipState::BS_ACTIVE:
		{
			if ( my_turn ) 
			{
				// TODO : on my turn . shots_fired
				for (Dot &shot : shots_fired) {
					display.draw_dot(shot);
				}

				// show ships we killed
				// * This will be sent from enemy when we kill a ship
				for (SHIP &vessel : ships_kiiled) {
					vessel.draw_destroyed();
				}
			} 
			else
			{
				// TODO : on their turn . show my ships and over lay incomming_shots
				for (SHIP &vessel : ships) {
					vessel.draw();
				}

				if ( demoMode ) {
					// demo is us firing on us
					for (Dot &shot : shots_fired) {
						display.draw_dot(shot);
					}
				} else {
					// game mode is them firing on us
					for (Dot &shot : incoming_shots) {
						display.draw_dot(shot);
					}
				}

				for (SHIP &vessel : ships) {
					vessel.draw_destroyed();
				}
			}
		}
		break;

		case BattleShipState::BS_ENDING:
		{
			// TODO : some form of animation??

			// Show Enemy Ships alive
			// * This would be sent when the game is over from enemy
			for (SHIP &vessel : ships) { // ships_survied
				vessel.draw();
			}

			for (Dot &shot : shots_fired) {
				display.draw_dot(shot);
			}

			// Ships killed overlayed
			for (SHIP &vessel : ships_kiiled) {
				vessel.draw_destroyed();
			}				
		}
		break;		
	}

}


uint8_t BattleShip::get_hits_left()
{
	uint8_t hitsLeft = 0;

	for (SHIP &vessel : ships) {
		hitsLeft += vessel.get_hits_left();
	}

	return hitsLeft;

/*	
	// Check each winning combination
	for (uint8_t i = 0; i < 8; ++i)
	{
		// If the initial position is not 0 (not empty)
		if (board[winning_combinations[i][0]] != 0 &&
			board[winning_combinations[i][0]] == board[winning_combinations[i][1]] &&
			board[winning_combinations[i][1]] == board[winning_combinations[i][2]])
		{
			// Set the winning combo so we can draw a line through it
			winning_combo = i;
			audio_player.play_note(11, 11, 1, 2);
			// Return the piece type in the first position - the winner (2 for cross, 1 for circle)
			return board[winning_combinations[i][0]];
		}
	}

	// If no moves are left - it's a draw, so we start again
	if (moves_left == 0)
		return 99;

	// No winner yet
	return 0;
*/	
}

void BattleShip::start_game()
{
//	player_piece = (BoardPiece)piece;
}

/*
void BattleShip::set_piece(uint8_t piece)
{
	if ((host_starts && !is_hosting()) || (!host_starts && is_hosting()))
	{
		player_piece = (BoardPiece)piece;

		info_printf("Setting player piece to %d\n", (BoardPiece)piece);

		display.clear();
		set_state(GameState::GAME_RUNNING);
	}
}
*/
void BattleShip::check_end_game()
{
	if ( bs_state == BattleShipState::BS_ENDING ) {
		return;
	}

	// we have no more ships to sink
	if ( get_hits_left() <= 0 ) {

		info_printf("Game Over\n");

		audio_player.play_note(11, 11, 1, 2);

		// TODO : set state to end game .. for now lets just reset
		change_game_state( BattleShipState::BS_ENDING );
		return;
	}

	// Error trapper
	if ( available_shots.empty() ) {
		// OMG .. how did we not kill all ships and have no shots ?
		info_printf("ERROR NO SHOTS AND SHIP HITS %d Left\n", get_hits_left() );

		// TODO : set state to end game .. for now lets just reset
		change_game_state( BattleShipState::BS_ENDING );
		return;
	}
}

void BattleShip::end_game()
{
	reset_game();
}

void BattleShip::reset_game()
{
	// reset all the data so we can start again

	// TODO : clear game stuff 

	host_starts = !host_starts;
	my_turn = false;

	// 
	set_state(GameState::GAME_MENU);
}

void BattleShip::kill_game()
{
	info_printf("Kill game while in %s\n", game_state_names[(uint8_t) get_state() ]);

	change_game_state( BattleShipState::BS_NONE );
}

void BattleShip::set_state(GameState s)
{
	info_printf("New State will be: %s\n", game_state_names[(uint8_t)s]);

	if (s == GameState::GAME_RUNNING)
	{
		touch.clear_all_touches();
	}

	MultiplayerGame::set_state(s);
}

void BattleShip::send_command(BS_DataType _type, uint8_t _control)
{
	battleShip.send_data(_type, _control, 0, 0, 0);
}

void BattleShip::send_data(BS_DataType _type, uint8_t _misc, uint8_t _x, uint8_t _y, uint8_t _id)
{
	bs_game_data_chunk_t data;
	data.data.gtype = (uint8_t)GAME_ID;	// Set ID, This will come from Core at some point 
	data.data.dtype = (uint8_t)_type;
	data.data.data_x = _x;
	data.data.data_y = _y;
	data.data.data_id = _id;
	data.data.data_misc = _misc;
	share_espnow.getInstance().send_gamedata(data.raw, Elements(data.raw));
}

bool BattleShip::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
	// TODO : make sure its for us ?
	if ( data_len != sizeof(bs_game_data_chunk_t) ) {
		return false;
	}

	bs_game_data_chunk_t *new_packet = reinterpret_cast<bs_game_data_chunk_t *>((uint8_t *)data);

	info_print("Data: ");
	for (int i = 0; i < data_len; i++)
	{
		info_printf("%d ", (uint8_t)data[i]);
	}

	info_println();
/*
	if ((DataType)data[0] == DataType::SEND_MOVE || (DataType)data[0] == DataType::END_GAME)
	{
		update_position((uint8_t)data[1], (BoardPiece)(uint8_t)data[2]);
	}
	else 
	{
		set_piece((BoardPiece)(uint8_t)data[1]);
	}
*/	

	return true;
}


void BattleShip::create_random_board()
{
	// Clear any previous ships
	ships.clear();

	noEdgeShips = !noEdgeShips;	// For now lets toggle to make it interesting
	noTouchingShips = true;

	for (const ShipType shipID : FLEET) { 

		// Make functiom ?
		bool shipAfloat = false;
		int shipLength = ((int) shipID)-1;		// Length is ENUM ID - 1

		while ( !shipAfloat ) {

			int startX = std::rand() % MATRIX_SIZE;
			int startY = std::rand() % MATRIX_SIZE;

			int direction = std::rand() % 4;
			int addX = 0;
			int addY = 0;

			// Build ship and clamp in 4 directions
			switch ( std::rand() % 4 ) {

				default:
					startY = constrain(startY-shipLength, (std::rand()&1), MATRIX_SIZE-1);
					addY = 1;
				break;

				case 1:
					startX = constrain(startX+shipLength, 0, MATRIX_SIZE-shipLength-1-(std::rand()&1));
					addX = 1;
				break;

				case 2:
					startY = constrain(startY+shipLength, 0, MATRIX_SIZE-shipLength-1-(std::rand()&1));
					addY = 1;
				break;

				case 3:
					startX = constrain(startX-shipLength, (std::rand()&1), MATRIX_SIZE-1);
					addX = 1;
				break;

			}

			// no Edge , keep ends of ship inside edge
			if ( noEdgeShips ) {			
				startX = constrain(startX, 1, MATRIX_SIZE-shipLength-2);
				startY = constrain(startY, 1, MATRIX_SIZE-shipLength-2);
			}
		
			bool touching = false;
			for (SHIP &vessel : ships) {
				touching = vessel.lineIntersect( startX, startY, shipLength, addX > addY, noTouchingShips );

				if ( touching ) {
					break;
				}
			}


			if ( touching ) {
				info_printf(" touching ship x: %d, y: %d, len: %d\n", startX, startY, shipLength);
				continue;
			}

			info_printf(" ship %d, x: %d, y: %d, len: %d\n",shipID , startX, startY, shipLength);					

			ships.push_back( SHIP(shipID,shipLength,startX, startY,  addX > addY , display.Color(SHIP_COLOR[shipID][0], SHIP_COLOR[shipID][1], SHIP_COLOR[shipID][2])) );

			shipAfloat = true;
		}
	}
}

void BattleShip::set_hosting(bool state)
{
    MultiplayerGame::set_hosting(state);

	info_printf("BattleShip host? %s\n", (state ? "YES" : "NO"));

	// Kill game if running
	battleShip.kill_game();

	audio_player.play_wav_queue("battleship");
}

SFX BattleShip::get_game_wave_file(const char *wav_name)
{
	SFX file;

	if (game_wav_files.count(wav_name)>0)
	{
		file = game_wav_files[wav_name];
	}

	return file;
}