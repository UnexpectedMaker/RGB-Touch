#pragma once

#define RGB_COLOR(r,g,b) (uint16_t)((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);

#include <map>
#include "../../audio/audio.h"
#include "../../frameworks/mp_game.h"
#include "game_battleship_vessel.h"

// This could be defined when we add the games to the core
// then commands only for that ID are sent to that game
const uint8_t GAME_ID = 0;

const uint16_t SHOT_COLOR = RGB_COLOR(0x7F,0x7F,0x7F);
const uint16_t HIT_COLOR = RGB_COLOR(0xCF,0x00,0x00);
const uint16_t KILL_COLOR = RGB_COLOR(0xFF,0x00,0x00);

const int DEBONCE = 100;	// 1/10 of a second 

const int AI_LOOK_BACK = 5;	// Look back 5 * ai level (easy/meduim/hard)

enum BattleShipState : uint8_t
{
	BS_NONE = 0,
	BS_BOARD_SETUP = 1,		// Board created 
	BS_BOARD_READY = 2,		// Pending user to accept board
	BS_AWAITING_ENEMY = 3,	// Board accepted, waiting on other player
	BS_ACTIVE = 4,			// Both boards setup and ready
	BS_ENDING = 5,			// After winner is announced
};

static String battleship_state_names[] =
{
	"NONE",
	"BOARD SETUP",
	"READY",
	"AWAITING",
	"ACTIVE",
	"ENDING",
};

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
};

struct bs_game_data_t
{
	uint8_t gtype;		// Future Core will define this based on registration 
	uint8_t dtype;
	uint8_t data_x;		// X - fire or ship start
	uint8_t data_y;		// Y - fire or ship start
	uint8_t data_id;	// ID - ship id , 0 means fire 
	uint8_t data_misc;	// direction - only used if ship
};

typedef union
{
	struct
	{
		bs_game_data_t data;
	} __attribute__((packed));

	uint8_t raw[sizeof(bs_game_data_t)];
} bs_game_data_chunk_t;

class BattleShip : public MultiplayerGame
{
	public:
		BattleShip();
    	~BattleShip() {
			game = nullptr;		
		}

		// Core overrides ?
		void set_state(GameState s) override;		
	   	void display_game() override;
		bool touched_board(uint8_t x, uint8_t y) override;
    	void update_loop() override;
		void kill_game() override;
		void set_hosting(bool state) override;

		bool onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) override;
		SFX get_game_wave_file(const char *wav_name) override;

	private:
		// anim vars for pre connected , might make this a demo mode
//		int16_t waiting_radius[2] = {0, 0};
//		uint16_t wait_period = 150;

		unsigned long next_display_update = 0;

		bool my_turn = false;
		bool host_starts = true;
		uint8_t players_ready = 0;		// 

		bool noEdgeShips = false;		// Ships are not allowed on edge if true, toggle on board create
		bool noTouchingShips = false;	// Ships cant touch if true

		std::vector<Point> lastTouch;

		std::map<const char *, SFX> game_wav_files;
		BattleShipState bs_state = BattleShipState::BS_NONE;

		// Player board info
		std::vector<SHIP> ships;
		std::vector<Dot> incoming_shots;

		// Enemy board info
		std::vector<Dot> shots_fired;
		std::vector<SHIP> ships_kiiled;
		std::vector<SHIP> ships_survied;

		// Temp vars for now
		int tempLastms = 0;
		uint8_t tempCounter = 0;

		// 
		void start_game();
		void reset_game();
		void end_game();	
		uint8_t get_hits_left();

		//
		void send_command(BS_DataType _type, uint8_t _control);
		void send_data(BS_DataType _type, uint8_t _misc, uint8_t _x, uint8_t _y, uint8_t _id);

		// Debounce for no touch process , move to touch process to handle release
		int lastPressedTime = 0;
		void process_last_touch(uint8_t x, uint8_t y);
		
		//
		void render_game_board(bool demoMode);
		void change_game_state(BattleShipState s);
		void create_random_board();
		void check_end_game();

		// Kinda AI ish thinking 
		int ai_level = 3;
		std::vector<Dot> available_shots;
};