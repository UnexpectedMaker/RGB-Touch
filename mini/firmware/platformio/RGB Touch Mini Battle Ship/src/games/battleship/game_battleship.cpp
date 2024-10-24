/*
 * A multi-player Battle Ship game
 *
 * Uses ESP-NOW for device to device communication

* TODO

Last Packet Sent to: F0:9E:9E:31:EE:D0 | Status: Delivery Fail
This should remove the peer 

Sometimes get a panic from espshare
Sometimes get a freese on send

Pause demo game while in menu settings?

Network 
Peer Join - retain ID 
Peer lost 

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

//#include "esp_debug_helpers.h"
//	esp_backtrace_print(2);

BattleShip battleShip;	// Create instance , so we can notify core

BattleShip::BattleShip()
{
	// TODO : Core to have multi game options
	game = this;	// Base class to take init
	
	// ADD SFX used by game
	game_wav_files = {
//		{"miss", SFX(voice_miss, sizeof(voice_miss))},
//		{"hit", SFX(voice_hit, sizeof(voice_hit))},
//		{"kill", SFX(voice_kill, sizeof(voice_kill))},
		{"gameover", SFX(gameover, sizeof(gameover))},
		{"battleship", SFX(battleship, sizeof(battleship))},
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
void BattleShip::change_game_state(BattleShipState new_state)
{
	info_printf("change_game_state FROM : %s to %s\n", battleship_state_names[(uint8_t)bs_state], battleship_state_names[(uint8_t)new_state]);
	info_printf("change_game_state State: %s\n", game_state_names[(uint8_t)get_state()]);

	if ( bs_state == new_state ) {
		info_printf(" ERROR ALREADY on Battle State : %s\n", battleship_state_names[(uint8_t)bs_state]);
		return;
	}

	// Always clear timeout on state change
	set_timeout(true);

	switch( new_state ) {

		// TODO : have a demo mode ?
		case BattleShipState::BS_NONE:
		{
			my_turn = false;

			set_state(GameState::GAME_WAITING);
		}
		break;


		case BattleShipState::BS_ACTIVE:
		{
			// If in MP mode
			if (share_espnow.getInstance().has_peer()) {
				set_timeout(false);
			}
		}
		break;

		case BattleShipState::BS_WAITING_REPLY:
		{
			set_timeout(false);
		}
		break;

		//
		case BattleShipState::BS_BOARD_SETUP:
		{
			// Make function
			{
				ai_level = ( std::rand() % 3 ) + 1;

				info_printf("create_random_board AI[%d] Board\n", ai_level);

				create_random_board();
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

			// Do we have a peer - Future will move this to onData revieve 
			if (share_espnow.getInstance().has_peer())
			{
				set_state(GameState::GAME_MENU);
				new_state = BattleShipState::BS_BOARD_READY;

				// Setup temp to be our counter before we lock the board
				animLastms = millis();
				animCounter = (int)( (MATRIX_SIZE*MS_SECOND)/500);	// every 500ms for 15seconds 
			}
			else 
			{
				// Demo go straight to BS Active while waiting
				new_state = BattleShipState::BS_ACTIVE;
				set_state(GameState::GAME_WAITING);
			}
		}
		break;

		case BattleShipState::BS_WAITING_ENEMY:
			my_turn = false;
			set_state(GameState::GAME_RUNNING);

			if ( !players_ready ) {
				send_data(BS_DataType::WAITING_TO_PLAY,(host_starts<<1)|(is_hosting()),0,0,0);
				info_printf("SEND WAITING_TO_PLAY %d , starts %d \n", is_hosting(), host_starts);
			}

			set_timeout(false);
		break;

		case BattleShipState::BS_SHOOTING:
		{
			animLastms = millis();

			if ( my_turn ) {
				// Anim frames .. resets to 4 - hard coded make consts
				animCounter = 4;
			} else {
				// TODO : ANIM an incoming shoot
				animCounter = 1;
//				animCounter = (int) ((MS_SECOND*10)/100);	// every 100ms for 10seconds
			}

//			info_printf("BS_SHOOTING: %d %d\n", animCounter, animLastms);

			set_timeout(false);
		}
		break;

		case BattleShipState::BS_MISS:
		{
			animLastms = millis();

			if ( !my_turn ) {
				// TODO : ANIM an MISS ? play Sound ?
				animCounter = 0;
			} else {
				// TODO : ANIM an MISS ? play Sound ?
				animCounter = 1;
//				animCounter = (int) ((MS_SECOND*10)/100);	// every 100ms for 10seconds
			}

//			info_printf("BS_MISS: %d %d\n", animCounter, animLastms);

		}
		break;

		case BattleShipState::BS_HIT:
		{
			animLastms = millis();

			if ( !my_turn ) {
				// TODO : ANIM an HIT ? play Sound ?
				animCounter = 0;
			} else {
				// TODO : ANIM an HIT ? play Sound ?
				animCounter = 1;
//				animCounter = (int) ((MS_SECOND*10)/100);	// every 100ms for 10seconds
			}

//			info_printf("BS_HIT: %d %d\n", animCounter, animLastms);

		}
		break;

		case BattleShipState::BS_DESTROYED:
		{
			animLastms = millis();

			if ( !my_turn ) {
				// TODO : ANIM an KILL ? play Sound ?
				animCounter = 0;
			} else {
				// TODO : ANIM an KILL ? play Sound ?
				animCounter = 1;
//				animCounter = (int) ((MS_SECOND*10)/100);	// every 100ms for 10seconds
			}

//			info_printf("BS_DESTROYED: %d %d\n", animCounter, animLastms);

		}
		break;

		case BattleShipState::BS_ENDING:
		{
			my_turn = false;

//			info_printf("BS_ENDING: %d %d\n", animCounter, animLastms);

			if ( timedOutReached ) {

				audio_player.play_wav_queue("gameover");

			} else if (get_state() == GameState::GAME_RUNNING) {

				// Sent any ships that are alive
				for (SHIP &vessel : ships) {

					if ( !vessel.isDestroyed() ) {
						// Slow deliver the alive ships
						delay(100);
						Dot d = vessel.getStartPos();
						send_data(BS_DataType::SHIP_ALIVE,vessel.getDirection(), d.x, d.y, vessel.getID());
					}
				}

				// This will mark the battle as ended , so only send if we have ship
				send_command(BS_DataType::TOGGLE_TURN);

				audio_player.play_wav_queue("gameover");
			}


			animLastms = millis();
			animCounter = timedOutReached ? 1 : (int) ((MS_SECOND*10)/MS_SECOND);

			// for next matchup swap who starts
//			host_starts = !host_starts;

			info_printf("BS_ENDING: %d\n", timedOutReached);
		}
		break;
	}

	bs_state = new_state;
	info_printf("change_game_state State TO : %s\n", battleship_state_names[(uint8_t)bs_state]);
//  info_printf("change_game_state State: %s\n", game_state_names[(uint8_t)get_state()]);
}

void BattleShip::process_last_touch(uint8_t x, uint8_t y)
{
	if (get_state() == GameState::GAME_MENU)
	{
		info_printf("process_last_touch Game State: %s\n", game_state_names[(uint8_t)get_state()]);

		switch(bs_state) {

			// reset if touched while in board ready 
			case BattleShipState::BS_BOARD_READY:
			{
				change_game_state( BattleShipState::BS_BOARD_SETUP );
			}
			break;
		}

		if ((host_starts && is_hosting()) || (!host_starts && !is_hosting()))
		{
			

/*
//					player_piece = BoardPiece::CROSS;
//					send_data(DataType::SET_PIECE, (uint8_t)BoardPiece::CIRCLE, 0);
enum BS_DataType : uint8_t
{
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

			if ( my_turn ) {

				bool clearToFire = true;
				for (Dot &shot : shots_fired) {
				
					if ( shot.x == x && shot.y == y ) {
						clearToFire = false;
						break;
					}
				}

				if ( clearToFire ) {

					// Make function ?
					info_printf("GAME TOUCH FIRE[%d] x: %d, y: %d - %d %s\n",my_turn , x, y , get_total_hits_left(), battleship_state_names[(uint8_t)bs_state]);
					audio_player.play_note(6, 11, 1.0, 1);

					shotResults = Dot(x, y, 0);

					send_data(BS_DataType::SHOT_FIRED,0,x,y,0);
					change_game_state( BattleShipState::BS_SHOOTING);

				} else {
					info_printf("GAME TOUCH NO FIRE[%d] x: %d, y: %d - %d %s\n",my_turn , x, y , get_total_hits_left(), battleship_state_names[(uint8_t)bs_state]);
					audio_player.play_note(0, 0, 1, 0.2);
				}

				// TODO : check if have fired here before 
				// * Send fire and return
			} else {
				info_printf("GAME TOUCH ACTIVE[%d] x: %d, y: %d - %d %s\n",my_turn , x, y , get_total_hits_left(), battleship_state_names[(uint8_t)bs_state]);
			}
		}
	}
}

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
			process_last_touch(lastTouch[0].x, lastTouch[0].y);
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
		// Stop demo if we have a peer : This needs to change so we get a message to say they are a BattleShip Game
		if (share_espnow.getInstance().has_peer()) {
			kill_game();
			return;
		} else if ( bs_state == BattleShipState::BS_NONE ) {
			// If we have no setup , lets create a board for AI/Demo Play
			change_game_state( BattleShipState::BS_BOARD_SETUP );		
		}
		
		// Waiting is Demo Mode for now
		if ( bs_state == BattleShipState::BS_ACTIVE ) {

// TODO : move this to its own AI class

			// 1/2 a Second fire rate
			if ( millis() - tempLastms >= 500 ) {
				tempLastms = millis();

				if ( check_end_game() ) {
					kill_game();
					return;
				}

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

				uint16_t fireColor = SHOT_COLOR;	

				SHIP* hitShip = CheckShipHit(shot.x, shot.y, false);

				if ( hitShip ) {
					if ( hitShip->isDestroyed() ) {
						ships_kiiled.push_back( hitShip->duplicate(true) );
						fireColor = KILL_COLOR;
					} else {
						fireColor = HIT_COLOR;
					}
				}
				
				// Set fired in demo
				Dot d = Dot(shot.x, shot.y, fireColor);
				shots_fired.push_back(d);

//				info_printf("Shoot x: %d, y: %d\n" , shot.x, shot.y );
			}
		}
	} else if (get_state() == GameState::GAME_MENU) {

		// Any other state we should kill game
		if ( bs_state == BattleShipState::BS_BOARD_READY ) {

			// trap to allow game to run (demo)			

		} else if ( bs_state == BattleShipState::BS_WAITING_ENEMY ) {

			// trap to allow game to run (demo)			

		} else {
			// TODO : know if the board needs to be killed ?
			// in menu reset board
//			kill_game();
		}

	} else if (get_state() == GameState::GAME_RUNNING) {

		if (timeoutCounter > 0 ) {

			if ( millis() - lastTimeoutms >= MS_SECOND ) {
				lastTimeoutms = millis();

				// Send waitin for enemy incase they joined later, every second
				if ( bs_state == BattleShipState::BS_WAITING_ENEMY ) {
					send_data(BS_DataType::WAITING_TO_PLAY,(host_starts<<1)|(is_hosting()),0,0,0);
//					info_printf("RE SEND WAITING_TO_PLAY %d , starts %d \n", is_hosting(), host_starts);
				}

				timeoutCounter--;
//				info_printf("TIMEOUT %d\n",timeoutCounter);

				if ( timeoutCounter == 0 ) {

					if ( bs_state != BattleShipState::BS_ENDING ) {
						info_printf("GAME_RUNNING TIMEOUT REACH\n");
						timeout_game();
						return;
					}
				}
			}
		}
	} 

	update_board_state();
}

void BattleShip::update_board_state()
{
	switch (bs_state) {

		// no State no board process
		case BattleShipState::BS_NONE:
			return;

		case BattleShipState::BS_BOARD_SETUP:
		break;

		case BattleShipState::BS_BOARD_READY:
			
			// TODO : const

			// Anim counter for bar
			if ( millis() - animLastms >= 500 ) 
			{
				animLastms = millis();

				animCounter--;

				if ( animCounter == 0 ) {
					info_printf("Board applied hosting %d , starts %d \n", is_hosting(), host_starts);
					change_game_state( BattleShipState::BS_WAITING_ENEMY );
				}
			}
		break;

		case BattleShipState::BS_WAITING_ENEMY:

			// Once they announce ready, lets start game
			if ( players_ready ) {
				info_printf("Players waiting - start game %d , starts %d \n", is_hosting(), host_starts);
				start_game();
			}

		break;

		case BattleShipState::BS_ACTIVE:
		{
			// TODO : check for lost connection/peer ?

			// TODO : logic on player ? 

		}
		break;

		// Anim Firing shot
		case BattleShipState::BS_SHOOTING:
		{
			if ( animCounter > 0 ) {
			
				if ( millis() - animLastms >= 100 ) {
					animLastms = millis();
					animCounter--;

					if ( my_turn && animCounter <= 0) {

						if ( shotResults.color == 0 ) {
							animCounter = 4;
						}
					}
				}
				break;
			}

			// on our turn , state change based on color 
			if ( my_turn ) {
					shots_fired.push_back(shotResults);
				if ( shotResults.color == KILL_COLOR ) {
					change_game_state( BattleShipState::BS_DESTROYED );
				} else if ( shotResults.color == HIT_COLOR ) {
					change_game_state( BattleShipState::BS_HIT);
				} else {
					change_game_state( BattleShipState::BS_MISS );
				}
				break;
			}

			info_printf("BS_SHOOTING: %d %d\n", animCounter, millis());

			// Check if shot hit our ships
//			Dot d = incoming_shots[incoming_shots.size()-1];

			shotResults.color = SHOT_COLOR;

			// Check for hit and reply
			SHIP* hitShip = CheckShipHit(shotResults.x, shotResults.y, false);

			if ( hitShip == nullptr ) {
				info_printf("update_board_state SEND SHOT MISS : %d %d\n", shotResults.x, shotResults.y );
				send_data(BS_DataType::SHOT_MISS,0,shotResults.x, shotResults.y,0);
			} else {

				info_printf("update_board_state SEND SHOT HIT : %d %d\n", shotResults.x, shotResults.y );
				send_data(BS_DataType::SHOT_HIT,hitShip->get_hits_left(),shotResults.x, shotResults.y,0); // optional ship id hitShip->getID());
				shotResults.color = HIT_COLOR;

				if ( hitShip->isDestroyed() ) {
					Dot sp = hitShip->getStartPos();
					info_printf("update_board_state SEND SHOT KILL : %d %d\n", sp.x, sp.y );
					send_data(BS_DataType::SHIP_KILLED,hitShip->getDirection(),sp.x, sp.y,hitShip->getID());
					shotResults.color = KILL_COLOR;
				}
			}

			incoming_shots.push_back(shotResults);

			change_game_state( BattleShipState::BS_WAITING_REPLY );
		}
		break;

		// Anim Miss Shot
		case BattleShipState::BS_MISS:
		{
			if ( animCounter > 0 ) {
			
				if ( millis() - animLastms >= MS_SECOND ) {
					animLastms = millis();
					animCounter--;
				}

				break;
			}

			// not our turn, anim only 
			if ( !my_turn ) {
				break;
			}

			info_printf("BS_MISS: %d %d\n", animCounter, millis());

			// End turn on miss 
			end_turn();
		}
		break;

		// Anim hit Shot
		case BattleShipState::BS_HIT:
		{
			if ( animCounter > 0 ) {
			
				if ( millis() - animLastms >= MS_SECOND ) {
					animLastms = millis();
					animCounter--;
				}


				break;
			}

			// not our turn, anim only 
			if ( !my_turn ) {
				break;
			}


			info_printf("BS_HIT: %d %d\n", animCounter, millis());

			// Allow to fire again
			change_game_state( BattleShipState::BS_ACTIVE );
		}
		break;

		// Anim Kill shot
		case BattleShipState::BS_DESTROYED:
		{
			if ( animCounter > 0 ) {
			
				if ( millis() - animLastms >= MS_SECOND ) {
					animLastms = millis();
					animCounter--;
				}


				break;
			}

			// not our turn, anim only 
			if ( !my_turn ) {
				break;
			}

			if ( enemy_killed ) {

				info_printf("BS_DESTROYED: end_gamen");
				kill_game();
	//			change_game_state( BattleShipState::BS_ACTIVE );

			} else {
				info_printf("BS_DESTROYED: %d %d\n", animCounter, millis());
				// TODO : end turn or allow two shots depending on game mode ?
				// Allow to fire again
				change_game_state( BattleShipState::BS_ACTIVE );
			}
		}
		break;

		case BattleShipState::BS_ENDING:
		{
			// TODO : some form of animation
			if ( millis() - animLastms >= MS_SECOND ) 
			{
				animLastms = millis();

				animCounter--;

				if ( animCounter == 0 ) {

					//
					info_printf("Game Over reset state\n");

					change_game_state( BattleShipState::BS_NONE );
				}
			}
		}
		break;
	}
}

SHIP* BattleShip::CheckShipHit(int x , int y, bool splash) 
{
	std::string sfx = "miss";

	for (SHIP &vessel : ships) {
		int8_t hitsLeft = vessel.hitCheck(x, y, splash);

		if ( hitsLeft >= 0 ) {
			if ( hitsLeft == 0 ) {
				audio_player.play_note(6, 11, 1.0, 1);
				sfx = "kill";
			} else {
				audio_player.play_note(0, 6, 1.0, 0.2);
				sfx = "hit";
			}

			return std::addressof(vessel);
		}
	}

	return nullptr;
}


void BattleShip::display_icon()
{
	// Future .. 
}

void BattleShip::display_game()
{
	if (get_state() == GameState::GAME_WAITING) {

		render_game_board();
	}
	else if (get_state() == GameState::GAME_MENU)
	{
		render_game_board();

		if ( bs_state == BattleShipState::BS_BOARD_READY ) {
			if ( animCounter&1 ) {
				display.draw_line(0,MATRIX_SIZE-1,(animCounter/2),MATRIX_SIZE-1, display.Color(128, 0, 0) );
			}
		}
	}
	else if (get_state() == GameState::GAME_RUNNING)
	{
		render_game_board();
	}
	else if (get_state() == GameState::GAME_ENDED)
	{
		info_printf("GAME_ENDED State: %s\n", game_state_names[(uint8_t)get_state()]);

		display.clear();
	}
}

void BattleShip::render_game_board()
{
	if ( bs_state == BattleShipState::BS_NONE ) {
		return;
	}

	bool demoMode = (get_state() == GameState::GAME_WAITING);

	display.clear();

	switch (bs_state) {

		case BattleShipState::BS_BOARD_SETUP:
		case BattleShipState::BS_BOARD_READY:
		case BattleShipState::BS_WAITING_ENEMY:
		{
			// Draw ships always
			for (SHIP &vessel : ships) {
				vessel.draw();
			}
		}
		break;

// TODO : animation on each state of attack
		case BattleShipState::BS_SHOOTING:
		{
			render_active_board(demoMode);

			if ( my_turn ) {

				int sx=shotResults.x;
				int sy=shotResults.y;
				int ex=shotResults.x;
				int ey=shotResults.y;

				// Anim on shooting - Spinning star
				switch ( animCounter%4 ) {
					case 0:
						sy = constrain(shotResults.y-1,0, MATRIX_SIZE-1);
						ey = constrain(shotResults.y+1,0, MATRIX_SIZE-1);
					break;

					case 1:
						sy = constrain(shotResults.y-1,0, MATRIX_SIZE-1);
						ey = constrain(shotResults.y+1,0, MATRIX_SIZE-1);
					case 2:
						sx = constrain(shotResults.x+1,0, MATRIX_SIZE-1);
						ex = constrain(shotResults.x-1,0, MATRIX_SIZE-1);
					break;

					case 3:
						sx = constrain(shotResults.x+1,0, MATRIX_SIZE-1);
						ex = constrain(shotResults.x-1,0, MATRIX_SIZE-1);
						sy = constrain(shotResults.y+1,0, MATRIX_SIZE-1);
						ey = constrain(shotResults.y-1,0, MATRIX_SIZE-1);
					break;
				}

				display.draw_line(sx,sy,ex,ey, SHOT_COLOR );

			} else {
				// TODO : Anim on incoming 

			}
		}
		break;

		case BattleShipState::BS_MISS:
		{
			render_active_board(demoMode);

		}
		break;

		case BattleShipState::BS_HIT:
		{
			render_active_board(demoMode);

		}
		break;

		case BattleShipState::BS_DESTROYED:
		{
			render_active_board(demoMode);

			Dot d = shots_fired[shots_fired.size()-1];

		}
		break;

		case BattleShipState::BS_WAITING_REPLY:
		case BattleShipState::BS_ACTIVE:
		{
			render_active_board(demoMode);
		}
		break;

		case BattleShipState::BS_ENDING:
		{
			// TODO : some form of animation??
			if (get_state() == GameState::GAME_RUNNING) {

				// Show thier ships that survived
				for (SHIP &vessel : ships_survied) {
					vessel.draw();
				}

				for (Dot &shot : shots_fired) {
					display.draw_dot(shot);
				}

				// Anim killed ships
				for (SHIP &vessel : ships_kiiled) {
					vessel.draw_destroyed();
				}

			} else {

				// Demo mode
				for (SHIP &vessel : ships) {
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

		}
		break;		
	}

}

void BattleShip::render_active_board(bool demoMode)
{
	// TODO : make function
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

		// TODO : Draw anim for the last touch 
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

uint8_t BattleShip::get_total_hits_left()
{
	uint8_t hitsLeft = 0;

	for (SHIP &vessel : ships) {
		hitsLeft += vessel.get_hits_left();
	}

	return hitsLeft;
}

void BattleShip::start_game()
{
	info_printf("start_game: %d\n", my_turn);

	if ((host_starts && is_hosting()) || (!host_starts && !is_hosting())) {
		my_turn = true;
		info_printf("start_game: MY TURN %d\n", my_turn);
	} else {
		my_turn = false;
		info_printf("start_game: THIER TURN %d\n", my_turn);
	}

	change_game_state( BattleShipState::BS_ACTIVE );

	set_timeout(false);
}

bool BattleShip::check_end_game()
{
	return ( get_total_hits_left() <= 0) ;
}

void BattleShip::reset_game()
{
	// reset all the data so we can start again
	host_starts = !host_starts;
	my_turn = false;

	// 
	set_state(GameState::GAME_MENU);
}

void BattleShip::kill_game()
{
	info_printf("kill_game while in %s\n", game_state_names[(uint8_t) get_state() ]);
	info_printf("kill_game Battle State - %s\n", battleship_state_names[(uint8_t)bs_state]);

	my_turn = false;

	if (get_state() == GameState::GAME_RUNNING) {
		info_printf("Force Game Over\n");
		change_game_state( BattleShipState::BS_ENDING );
	} else {
		info_printf("New Game\n");
		set_state(GameState::GAME_MENU);
		change_game_state( BattleShipState::BS_BOARD_SETUP );
	}
}

void BattleShip::set_state(GameState s)
{
	info_printf("BATTLESHIP New State will be: %s\n", game_state_names[(uint8_t)s]);

	if (s == GameState::GAME_RUNNING)
	{
		touch.clear_all_touches();
	}

	MultiplayerGame::set_state(s);
}

void BattleShip::send_command(BS_DataType _type)
{
	send_data(_type, 0, 0, 0, 0);
}

void BattleShip::send_control(BS_DataType _type, uint8_t _control)
{
	send_data(_type, _control, 0, 0, 0);
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
	data.data.data_total_hits = get_total_hits_left();

	share_espnow.getInstance().send_gamedata(data.raw, Elements(data.raw));
}

bool BattleShip::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
	// TODO : make sure its for us ?
	if ( data_len != sizeof(bs_game_data_chunk_t) ) {
		return false;
	}

	bs_game_data_chunk_t *new_packet = reinterpret_cast<bs_game_data_chunk_t *>((uint8_t *)data);
/*
	info_print("Mac_addr: ");
	for (int i = 0; i < 6; i++)
	{
		info_printf("%02X", mac_addr[i]);
		if (i < 5)
			info_print(":");
	}
*/	
/*
	info_printf("onDataRecv Game State: %s\n", game_state_names[(uint8_t)get_state()]);
	info_printf("onDataRecv Battle State - %s\n", battleship_state_names[(uint8_t)bs_state]);

	info_printf("onDataRecv Data::gtype %d\n", new_packet->data.gtype);
	info_printf("onDataRecv Data::dtype %d\n", new_packet->data.dtype);
	info_printf("onDataRecv Data::data_x %d\n", new_packet->data.data_x);
	info_printf("onDataRecv Data::data_y %d\n", new_packet->data.data_y);
	info_printf("onDataRecv Data::data_id %d\n", new_packet->data.data_id);
	info_printf("onDataRecv Data::data_misc %d\n", new_packet->data.data_misc);
	info_printf("onDataRecv Data::data_total_hits %d\n", new_packet->data.data_total_hits);

	info_printf("onDataRecv players_ready %d\n", players_ready);
	info_printf("onDataRecv my_turn %d\n", my_turn);
	info_printf("onDataRecv timeout %d\n", timeoutCounter);
*/
	if ( my_turn ) 
	{
		// If my turn options
		switch ( (BS_DataType) new_packet->data.dtype ) {
			
			case BS_DataType::SHOT_MISS:
			{
//				info_printf("onDataRecv ACTIVE SHOT MISS : %d %d\n", new_packet->data.data_x, new_packet->data.data_y );
				shotResults.color = SHOT_COLOR;
			}
			break;

			case BS_DataType::SHOT_HIT:
			{
//				info_printf("onDataRecv ACTIVE SHOT HIT : %d %d\n", new_packet->data.data_x, new_packet->data.data_y );

				shotResults.color = HIT_COLOR;
			}
			break;

			case BS_DataType::SHIP_KILLED:
			{
//				info_printf("onDataRecv ACTIVE SHOT KILL[%d] : %d %d - hits [%d]\n",new_packet->data.data_id , new_packet->data.data_x, new_packet->data.data_y, new_packet->data.data_total_hits );

				// Add to killed ship table for animation
				int shipID = new_packet->data.data_id;
				uint8_t shipLength = ((uint8_t) shipID)-1;

				// Add a killed ship to vector
				ships_kiiled.push_back( SHIP((ShipType)new_packet->data.data_id, shipLength, new_packet->data.data_x, new_packet->data.data_y,  new_packet->data.data_misc , display.Color(SHIP_COLOR[shipID][0], SHIP_COLOR[shipID][1], SHIP_COLOR[shipID][2]), true ) );

				enemy_killed = ( new_packet->data.data_total_hits == 0 ) ? true : false;

				shotResults.color = KILL_COLOR;
			}
			break;

			case BS_DataType::TOGGLE_TURN:
			{
				info_printf("onDataRecv ACTIVE TOGGLE_TURN : %d\n", new_packet->data.data_misc );
				my_turn = false;

				if ( new_packet->data.data_misc ) {
					kill_game();
				} else {
					change_game_state( BattleShipState::BS_ACTIVE );
					info_printf("TURN_CHANGE my_turn: %d\n", my_turn);			
				}
			}
			break;

			case BS_DataType::TIMED_OUT:
				info_printf("onDataRecv TIMED_OUT\n" );

				// If we are waiting and they toggled turn they timed out
				info_printf("Re Init Game Over\n");
				timeout_game();
			break;

			default:
				info_printf("onDataRecv ACTIVE TYPE[%d] UNKOWN : %d\n", new_packet->data.dtype,my_turn );
			break;
		}

	} else {

		// If not my turn options
		switch ( (BS_DataType) new_packet->data.dtype ) {
			
// 	 = 0,		// FUIURE to force them into this game mode
			case BS_DataType::WANT_TO_PLAY:
			{

			}
			break;

			case BS_DataType::WAITING_TO_PLAY:

			{
				bool theyHost = (new_packet->data.data_misc&1) ? true : false;
				bool theyStart = (new_packet->data.data_misc>>1)&1 ? true : false;
/*/
				info_printf("onDataRecv WAITING_TO_PLAY They host %d , they starts %d \n", theyHost, theyStart);
				info_printf("We host %d , we starts %d \n", is_hosting(), host_starts);
*/
				// ignore new peers, asborb data
				if ( players_ready ) {
					info_printf("onDataRecv Ready Players already full : %d\n", players_ready);
					return true;
				}

				players_ready =  true;
			}
			break;

			case BS_DataType::SHOT_FIRED:
			{
				if (get_state() != GameState::GAME_RUNNING) {
					break;
				}

				info_printf("onDataRecv INACTIVE INCOMMING_SHOT  : %d %d\n", new_packet->data.data_x, new_packet->data.data_y );
				shotResults = Dot(new_packet->data.data_x, new_packet->data.data_y, 0);
				change_game_state( BattleShipState::BS_SHOOTING);
			}
			break;

			case BS_DataType::TOGGLE_TURN:
			{
				if (get_state() != GameState::GAME_RUNNING) {
					break;
				}

				info_printf("onDataRecv INACTIVE TOGGLE_TURN : %d %d\n", new_packet->data.data_total_hits, get_total_hits_left() );

				// If we have end game or they have no hits left
				if ( check_end_game() || new_packet->data.data_total_hits == 0 ) {

					info_printf("Game Over\n");
					kill_game();

				} else {
					
					my_turn = true;
					change_game_state( BattleShipState::BS_ACTIVE );
				}
				info_printf("TURN_CHANGE my_turn: %d\n", my_turn);
			}
			break;

			case BS_DataType::SHIP_ALIVE:
			{
				if (get_state() != GameState::GAME_RUNNING) {
					break;
				}

				// Add to alive ships table for animation
				int shipID = new_packet->data.data_id;
				uint8_t shipLength = ((uint8_t) shipID)-1;

				ships_survied.push_back( SHIP((ShipType)new_packet->data.data_id, shipLength, new_packet->data.data_x, new_packet->data.data_y,  new_packet->data.data_misc , display.Color(SHIP_COLOR[shipID][0], SHIP_COLOR[shipID][1], SHIP_COLOR[shipID][2]), false ) );

				info_printf("SHIP_ALIVE my_turn: %d\n", my_turn);
			}
			break;

			case BS_DataType::TIMED_OUT:
				info_printf("onDataRecv INACTIVE TIMED_OUT\n" );

				// If we are waiting and they toggled turn they timed out
				info_printf("Re Init Game Over\n");
				kill_game();
			break;

			default:
				info_printf("onDataRecv INACTIVE TYPE[%d] UNKOWN : %d\n", new_packet->data.dtype,my_turn );
			break;
		}
	}

	info_printf("onDataRecv After Game State: %s\n", game_state_names[(uint8_t)get_state()]);
	info_printf("onDataRecv After Battle State - %s\n", battleship_state_names[(uint8_t)bs_state]);

	return true;
}

void BattleShip::end_turn()
{
	info_printf("end_turn Game State: %s - hits left %d\n", game_state_names[(uint8_t)get_state()], get_total_hits_left());
	info_printf("end_turn Battle State - %s - hits left %d\n", battleship_state_names[(uint8_t)bs_state], get_total_hits_left());

	my_turn = false;

	// If not end, send player turn toggle with hits we have left
	send_command(BS_DataType::TOGGLE_TURN);
	change_game_state( BattleShipState::BS_ACTIVE );
}

void BattleShip::create_random_board()
{
	// Clear game data before we create a new board
	ships.clear();
	shots_fired.clear();
	incoming_shots.clear();
	ships_kiiled.clear();
	ships_survied.clear();

	timedOutReached = false;
	enemy_killed = false;
	my_turn = false;
	players_ready = false;

	noEdgeShips = !noEdgeShips;	// For now lets toggle to make it interesting
	noTouchingShips = true;

	for (const ShipType shipID : FLEET) { 

		// Make functiom ?
		bool shipAfloat = false;
		int shipLength = ((int) shipID)-1;		// Length is ENUM ID - 1

		info_printf(" Trying ship[%d] len: %d\n",shipID, shipLength);

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
			
			// If we dont allow touching
			if ( noTouchingShips ) {

				// Find if this ship will touch any others with expanded boxes
				for (SHIP &vessel : ships) {
					touching = vessel.lineIntersect( startX, startY, shipLength, addX > addY, noTouchingShips );
					if ( touching ) {
						break;
					}
				}
			}


			if ( !touching ) {
				info_printf(" ship %d, x: %d, y: %d, len: %d\n",shipID , startX, startY, shipLength);					

				ships.push_back( SHIP(shipID,shipLength,startX, startY,  addX > addY , display.Color(SHIP_COLOR[shipID][0], SHIP_COLOR[shipID][1], SHIP_COLOR[shipID][2])) );

				shipAfloat = true;
			}
		}
	}
}

void BattleShip::timeout_game()
{
	// Kill peers ?
	info_printf("timeout_game\n");

	if (get_state() <= GameState::GAME_MENU && bs_state <= BattleShipState::BS_BOARD_SETUP ) {
		return;
	}

	timedOutReached = true;

	send_command(BS_DataType::TIMED_OUT);

	kill_game();

	set_state(GameState::GAME_MENU);
	change_game_state( BattleShipState::BS_NONE );
}

void BattleShip::set_timeout(bool reset)
{
	timeoutCounter = reset ? 0 : MAX_TIMEOUT;
	info_printf("onDataRecv timeout[%d] %d\n", reset, timeoutCounter );
}

void BattleShip::set_hosting(bool state)
{
    MultiplayerGame::set_hosting(state);

	info_printf("BattleShip host? %s\n", (state ? "YES" : "NO"));

	// Kill game if running
	kill_game();

	audio_player.play_wav_queue("battleship");
}

void BattleShip::peer_added(const uint8_t *mac_addr)
{
	MultiplayerGame::peer_added(mac_addr);

	info_printf("BattleShip Peer Added\n");
}

void BattleShip::peer_removed(const uint8_t *mac_addr)
{
	MultiplayerGame::peer_removed(mac_addr);

	info_printf("BattleShip Peer Removed\n");
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