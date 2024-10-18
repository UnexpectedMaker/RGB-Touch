#pragma once

#define RGB_COLOR(r,g,b) (uint32_t)((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);

#include <map>
#include "../../audio/audio.h"
#include "../../frameworks/mp_game.h"
#include "game_battleship_vessel.h"

// This could be defined when we add the games to the core
// then commands only for that ID are sent to that game
const uint8_t GAME_ID = 0;

enum BattleShipState : uint8_t
{
	BS_NONE = 0,
	BS_BOARD_SETUP = 1,		// Board created 
	BS_BOARD_READY = 2,		// Pending user to accept board
	BS_ACTIVE = 3,			// Both boards setup and ready
	BS_ENDING = 4,			// After winner is announced
};

static String battleship_state_names[] =
{
	"NONE",
	"BOARD SETUP",
	"READY",
	"ACTIVE",
	"ENDING",
};

enum BS_DataType : uint8_t
{
	WANT_TO_PLAY = 0,
	SETUP_BOARD = 1, 
	
	// TODO : more User is building board
	// * allow user to rotate pieces
	// ** Tap on piece rotates
	// * allow user to move pieces 
	// ** Select first then select area to move top or left most section

	SEND_MOVE = 2, // More user fire at square
	SEND_DESTROYED_SHIP = 3,

	// 
	SEND_ALIVE_SHIP = 4,

	END_GAME = 5,
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
		int16_t waiting_radius[2] = {0, 0};
		uint16_t wait_period = 150;

		unsigned long next_display_update = 0;

		bool my_turn = false;
		bool host_starts = true;

		bool noEdgeShips = true;		// Ships are not allowed on edge if true
		bool noTouchingShips = true;	// Ships cant touch if true

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

		void start_game();
		void reset_game();
		void end_game();	
		uint8_t getHitsLeft();

		void send_data(BS_DataType _type, uint8_t _x, uint8_t _y, uint8_t _id, uint8_t _misc);

		// Debounce for no touch process , move to touch process to handle release
		int lastPressedTime = 0;
		void processLastTouch(uint8_t x, uint8_t y);
		
		//
		void renderGameBoard(bool demoMode);
		void change_game_state(BattleShipState s);
		void createRandomBoard();
		void check_end_game();

		// Kinda AI ish thinking 
		std::vector<Dot> available_shots;
};