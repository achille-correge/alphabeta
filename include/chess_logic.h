#ifndef CHESS_LOGIC_H
#define CHESS_LOGIC_H

#include <stdbool.h>
#include <stdio.h>
#include "types.h"
#include "legal_moves.h"

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
PositionList *empty_list();
void free_position_list(PositionList *pos_l);
int pos_list_length(PositionList *pos_l);
PositionList *save_position(BoardState *board_s, PositionList *pos_l);
Piece get_piece(Piece board[8][8], Coords coords);

// nul check
bool are_same_pos(BoardState *board_s1, BoardState *board_s2);
bool threefold_repetition(BoardState *board_s, PositionList *pos_l, int number_of_repetitions);
bool threefold_hash(uint64_t hash, PositionList *pos_l, int number_of_repetitions);
bool insufficient_material(BoardState *board_s);

// check functions
bool is_attacked(BoardState *board_s, Coords co, char color, bool check_would_stop);
bool is_check(BoardState *board_s, char color);
bool is_mate(BoardState *board_s, char color);
bool is_checkmate(BoardState *board_s, char color);
bool is_stalemate(BoardState *board_s, char color);

// move functions
BoardState *move_pawn_handling(BoardState *board_s, Piece move_piece, Piece dest_piece, Move sel_move);
BoardState *move_king_handling(BoardState *board_s, Piece move_piece, Coords init_coords, Coords new_coords);
BoardState *move_rook_handling(BoardState *board_s, Piece move_piece, Coords init_coords, Coords new_coords);
BoardState *move_piece(BoardState *board_s, Move sel_move);
BoardState *move_piece_forced(BoardState *board_s, Move sel_move);

// test move functions
bool can_move_heuristic(BoardState *board_s, Piece piece, Coords init_co, Coords dest_co, bool check_would_stop);
bool can_move(BoardState *board_s, Piece piece, Coords init_co, Coords dest_co, bool check_would_stop);

#endif