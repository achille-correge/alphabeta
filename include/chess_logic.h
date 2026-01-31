#ifndef CHESS_LOGIC_H
#define CHESS_LOGIC_H

#include <stdbool.h>
#include <stdio.h>
#include "types.h"

// basic functions
BoardState *init_board();
BoardState *FEN_to_board(char *FEN);
Piece empty_piece();
Coords empty_coords();
Move empty_move();
bool is_empty(Piece piece);
bool is_empty_coords(Coords coords);
bool is_empty_move(Move move);
int coords_to_square(Coords co);
Coords square_to_coords(int square);
PieceType char_to_piece_type(char c);
char piece_type_to_char(PieceType type);
PositionList *empty_list();
void free_position_list(PositionList *pos_l);
PositionList *save_position(BoardState *board_s, PositionList *pos_l);
Piece get_piece(Piece board[8][8], Coords coords);

// nul check
bool threefold_hash(uint64_t hash, PositionList *pos_l, int number_of_repetitions);
bool insufficient_material(BoardState *board_s);

// move functions
BoardState *move_pawn_handling(BoardState *board_s, Piece move_piece, Piece dest_piece, Move sel_move);
BoardState *move_king_handling(BoardState *board_s, Piece move_piece, Coords init_coords, Coords new_coords);
BoardState *move_rook_handling(BoardState *board_s, Piece move_piece, Coords init_coords, Coords new_coords);
BoardState *move_piece(BoardState *board_s, Move sel_move);

#endif