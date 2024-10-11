#pragma once

#include <map>
#include "../../audio/audio.h"
#include "../../frameworks/mp_game.h"

enum DataType : uint8_t
{
	WANT_TO_PLAY = 0,
	SET_PIECE = 1,
	SEND_MOVE = 2,
	END_GAME = 3,
};

enum BoardPiece : uint8_t
{
	EMPTY = 0,
	CIRCLE = 1,
	CROSS = 2,
};
struct game_data_t
{
		uint8_t dtype;
		uint8_t data0;
		uint8_t data1;
};

typedef union
{
		struct
		{
				game_data_t data;
		} __attribute__((packed));

		uint8_t raw[3];
} game_data_chunk_t;

class TicTacToe : public MultiplayerGame
{
	public:
		TicTacToe();
    	~TicTacToe() {
			game = nullptr;		
		}

		bool set_position(uint8_t position, uint8_t piece);

		uint8_t check_winner();
		void send_data(DataType type, uint8_t data0, uint8_t data1);

		// Virtual functions override 
		void start_game(uint8_t piece) override;
		void reset_game() override;
		void end_game() override;

		void set_state(GameState s) override;
	   	void display_game() override;
		bool touched_board(uint8_t x, uint8_t y) override;

		void update_position(uint8_t position, uint8_t piece) override;
		void set_piece(uint8_t piece) override;
		bool onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) override;
		SFX get_game_wave_file(const char *wav_name) override;

		int16_t waiting_radius[2] = {0, 0};
		uint16_t wait_period = 150;

	private:
		std::map<const char *, SFX> game_wav_files;

		uint8_t board[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint8_t pos_fader[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint8_t piece_pos_x[9] = {0, 4, 8, 0, 4, 8, 0, 4, 8};
		uint8_t piece_pos_y[9] = {0, 0, 0, 4, 4, 4, 8, 8, 8};
		uint8_t winning_player = 0;
		uint8_t winner_flash_counter = 10;
		uint8_t winning_combo = 255;

		uint8_t moves_left = 9;

		unsigned long next_display_update = 0;

		bool flashing_winner = false;
		BoardPiece player_piece = BoardPiece::EMPTY;
		bool my_turn = false;
		bool host_starts = true;

		// Winning combinations
		const uint8_t winning_combinations[8][3] = {
			{0, 1, 2}, // Row 1
			{3, 4, 5}, // Row 2
			{6, 7, 8}, // Row 3
			{0, 3, 6}, // Column 1
			{1, 4, 7}, // Column 2
			{2, 5, 8}, // Column 3
			{0, 4, 8}, // Diagonal 1
			{2, 4, 6}  // Diagonal 2
		};

		const uint8_t winning_centers[8][4] = {
			{1, 1, 9, 1}, // Row 1
			{1, 5, 9, 5}, // Row 2
			{1, 9, 9, 9}, // Row 3
			{1, 1, 1, 9}, // Column 1
			{5, 1, 5, 9}, // Column 2
			{9, 1, 9, 9}, // Column 3
			{1, 1, 9, 9}, // Diagonal 1
			{9, 1, 1, 9}  // Diagonal 2
		};
};

extern TicTacToe ticTacToe;