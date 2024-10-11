/*
 * A multi-player Tic Tac Toe game
 *
 * Uses ESP-NOW for device to device communication
 */

#include "../../display/display.h"
#include "../../touch/touch.h"
#include "../../share/share.h"

// Game specfic files
#include "game_tictactoe.h"
#include "audio/voice/voice_tic_tac_toe.h"

TicTacToe::TicTacToe()
{
	game = &ticTacToe;	// Base class to take init

	game_wav_files = {
		{"tictactoe", SFX(voice_tic_tac_toe, sizeof(voice_tic_tac_toe))},
	};

	// for (size_t i = 0; i < 9; i++)
	// 	board[i] = 0;
}

void TicTacToe::send_data(DataType type, uint8_t data0, uint8_t data1)
{
	game_data_chunk_t data;
	data.data.dtype = (uint8_t)type;
	data.data.data0 = data0;
	data.data.data1 = data1;
	share_espnow.getInstance().send_gamedata(data.raw, Elements(data.raw));
}

bool TicTacToe::touched_board(uint8_t x, uint8_t y)
{
	if (winning_player > 0)
		return false;

	if (moves_left == 0)
		return false;

	if (get_state() == GameState::GAME_MENU)
	{
		if ((host_starts && is_hosting()) || (!host_starts && !is_hosting()))
		{
			if (player_piece == BoardPiece::EMPTY)
			{
				if (x < 6)
				{
					player_piece = BoardPiece::CROSS;
					send_data(DataType::SET_PIECE, (uint8_t)BoardPiece::CIRCLE, 0);
				}
				else
				{
					player_piece = BoardPiece::CIRCLE;
					send_data(DataType::SET_PIECE, (uint8_t)BoardPiece::CROSS, 0);
				}

				my_turn = true;
				display.clear();
				set_state(GameState::GAME_RUNNING);
			}
		}
		return true;
	}
	else if (get_state() == GameState::GAME_RUNNING)
	{

		uint8_t pos = x / 4 + ((y / 4) * 3);
		// info_printf("x: %d, y: %d, pos: %d\n", x, y, pos);
		return (set_position(pos, player_piece));
	}

	return false;
}

bool TicTacToe::set_position(uint8_t position, uint8_t piece)
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

void TicTacToe::update_position(uint8_t position, uint8_t piece)
{
	if (position < 9)
	{
		if (board[position] == (uint8_t)BoardPiece::EMPTY)
		{
			board[position] = piece;
			pos_fader[position] = 0;

			moves_left--;

			winning_player = check_winner();

			if (winning_player == 0)
				my_turn = true;
		}
	}
}

void TicTacToe::display_game()
{
	if (get_state() == GameState::GAME_WAITING)
	{
		if (millis() - next_display_update > wait_period)
		{
			next_display_update = millis();

			display.fade_leds_to(80);
			if (!share_espnow.getInstance().has_peer())
			{
				display.draw_line(waiting_radius[0], 0, waiting_radius[0], 11, display.Color(128, 0, 0));
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
	}
	else if (get_state() == GameState::GAME_MENU)
	{
		display.clear();
		if ((host_starts && is_hosting()) || (!host_starts && !is_hosting()))
		{
			display.show_text(4, 5, "?", display.Color(50, 50, 255));

			display.show_icon(&display.icons["cross_small"], 1, 7, 255);
			display.show_icon(&display.icons["circle_small"], 7, 7, 255);
		}
		else
		{
			display.show_icon(&display.icons["waiting"], 2, 2, 255);
			display.icons["waiting"].cycle_frame();
		}
	}
	else if (get_state() == GameState::GAME_RUNNING)
	{
		display.clear();

		uint32_t line_col = (my_turn ? display.Color(220, 220, 220) : display.Color(100, 100, 100));

		// Vertical Lines
		display.draw_line(3, 0, 3, 10, line_col);
		display.draw_line(7, 0, 7, 10, line_col);
		// Horizontal Lines
		display.draw_line(0, 3, 10, 3, line_col);
		display.draw_line(0, 7, 10, 7, line_col);

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
	}
	else if (get_state() == GameState::GAME_ENDED)
	{
		display.clear();
	}
}

uint8_t TicTacToe::check_winner()
{
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
}

void TicTacToe::start_game(uint8_t piece)
{
	player_piece = (BoardPiece)piece;
}

void TicTacToe::set_piece(uint8_t piece)
{
	if ((host_starts && !is_hosting()) || (!host_starts && is_hosting()))
	{
		player_piece = (BoardPiece)piece;

		info_printf("Setting player piece to %d\n", piece);

		display.clear();
		set_state(GameState::GAME_RUNNING);
	}
}

void TicTacToe::end_game()
{
	reset_game();
}

void TicTacToe::reset_game()
{
	// reset all the data so we can start again
	player_piece = BoardPiece::EMPTY;
	host_starts = !host_starts;
	winner_flash_counter = 10;
	winning_player = 0;
	winning_combo = 0;
	my_turn = false;
	moves_left = 9;
	for (int b = 0; b < 9; b++)
		board[b] = 0;
	set_state(GameState::GAME_MENU);
}

void TicTacToe::set_state(GameState s)
{
	info_printf("New State will be: %s\n", game_state_names[(uint8_t)s]);

	if (s == GameState::GAME_RUNNING)
	{
		touch.clear_all_touches();
	}

	MultiplayerGame::set_state(s);
}

bool TicTacToe::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
	game_data_chunk_t *new_packet = reinterpret_cast<game_data_chunk_t *>((uint8_t *)data);

	info_print("Data: ");
	for (int i = 0; i < data_len; i++)
	{
		info_printf("%d ", (uint8_t)data[i]);
	}

	info_println();

	if ((DataType)data[0] == DataType::SEND_MOVE || (DataType)data[0] == DataType::END_GAME)
	{
		update_position((uint8_t)data[1], (BoardPiece)(uint8_t)data[2]);
	}
	else 
	{
		set_piece((BoardPiece)(uint8_t)data[1]);
	}
	

	return true;
}

SFX TicTacToe::get_game_wave_file(const char *wav_name)
{
	SFX file;

	if (game_wav_files.count(wav_name)>0)
	{
		file = game_wav_files[wav_name];
	}

	return file;
}


TicTacToe ticTacToe;