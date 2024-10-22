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

const int DEBONCE = 100;					// 1/10 of a second 
const int MS_SECOND = 1000;	
const int AI_LOOK_BACK = 5;					// Look back 5 * ai level (easy/meduim/hard)
const int MAX_TIMEOUT = (60*MS_SECOND);		// 60 seconds , if we dont get a play or battle we will re-cycle

const ShipType FLEET[] = {	// Reverse to give bigger ships a chance
	AIRCRAFT,
	BATTLESHIP,
	CRUISER,
	DESTROYER, 
	DESTROYER,
	SUBMARINE,
	SUBMARINE,
};

enum BattleShipState : uint8_t
{
	BS_NONE = 0,
	BS_BOARD_SETUP = 1,		// Board created 
	BS_BOARD_READY = 2,		// Pending user to accept board
	BS_WAITING_ENEMY = 3,	// Board accepted, waiting on other player
	BS_ACTIVE = 4,			// Both boards setup and ready
	BS_WAITING_REPLY = 5,	//
	BS_SHOOTING = 6,		// Anim incoming, swap to miss/hit/destroy
	BS_MISS = 7,			// Anim incoming, swap to miss/hit/destroy
	BS_HIT = 8,				// Anim incoming, swap to miss/hit/destroy
	BS_DESTROYED = 9,			// Anim incoming, swap to miss/hit/destroy

	BS_ENDING = 10,			// After winner is announced
};

static String battleship_state_names[] =
{
	"NONE",
	"BOARD SETUP",
	"READY",
	"WAITING ENEMY",
	"ACTIVE",
	"WAITING REPLY",
	"SHOOTING",
	"MISS",
	"HIT",
	"DESTROYED",

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

	SHOT_FIRED = 2,
	SHOT_MISS = 3, 
	SHOT_HIT = 4, 		// _misc == hit count left
	SHIP_KILLED = 5, 	// _misc == ship direction

	TOGGLE_TURN = 6,	// _misc == hits left, 0 means game over to winner
	// End of game winner sends ships not destroyed

	SHIP_ALIVE = 7,
};

struct bs_game_data_t
{
	uint8_t gtype;		// Future Core will define this based on registration 
	uint8_t dtype;
	uint8_t data_x;		// X - fire or ship start
	uint8_t data_y;		// Y - fire or ship start
	uint8_t data_id;	// ID - ship id , 0 means fire 
	uint8_t data_misc;	// direction - only used if ship
	uint8_t data_total_hits;	// how many hits are left for the board
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
		void display_icon() override;
		bool touched_board(uint8_t x, uint8_t y) override;
    	void update_loop() override;
		void kill_game() override;
		void set_hosting(bool state) override;
		void peer_added(const uint8_t *mac_addr) override;
		void peer_removed(const uint8_t *mac_addr) override;
		bool onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) override;
		SFX get_game_wave_file(const char *wav_name) override;

	private:
		// anim vars for pre connected , might make this a demo mode
//		int16_t waiting_radius[2] = {0, 0};
//		uint16_t wait_period = 150;

		unsigned long next_display_update = 0;

		bool enemy_killed = false;

		bool my_turn = false;
		bool host_starts = true;
		bool players_ready = false;		// 

		bool noEdgeShips = false;		// Ships are not allowed on edge if true, toggle on board create
		bool noTouchingShips = false;	// Ships cant touch if true
		bool timedOutReached = false;

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

		//
		int animLastms = 0;
		int animCounter = 0;
		Dot shotResults;
		long nextTimeout = 0;

		// Temp vars 
		long tempLastms = 0;
		int tempCounter = 0;

		// 
		void start_game();
		void reset_game();
		void end_game();	
		uint8_t get_total_hits_left();

		//
		void send_command(BS_DataType _type);
		void send_control(BS_DataType _type, uint8_t _control);
		void send_data(BS_DataType _type, uint8_t _misc, uint8_t _x, uint8_t _y, uint8_t _id);

		// Debounce for no touch process , move to touch process to handle release
		long lastPressedTime = 0;
		void process_last_touch(uint8_t x, uint8_t y);
		
		//
		void render_game_board();
		void render_active_board(bool demoMode);
		void update_board_state();
		void change_game_state(BattleShipState s);
		void create_random_board();
		bool check_end_game();
		void end_turn();
		SHIP* CheckShipHit(int x , int y, bool splash);
		void set_timeout(bool reset);
		void timeout_game();


		// Kinda AI ish thinking 
		int ai_level = 3;
		std::vector<Dot> available_shots;
};