#ifndef DEBUG_FUNCTIONS_H
#define DEBUG_FUNCTIONS_H

#include <stdio.h>
#include "types.h"

void print_bitboard(Bitboard b);
void print_move(Move move);
void print_move_list(MoveList *move_list);
void print_board_debug(BoardState *board_s);
Bitboard get_targetbb_move_list(MoveList *move_list);
bool is_in_move_list(MoveList *move_list, Move move);
bool are_same_move_set(MoveList *move_list, MoveList *move_list_bb);
void print_differences(MoveList *move_list, MoveList *move_list_bb);
void verify_and_print_differences(MoveList *move_list, MoveList *move_list_bb, PositionList *board_history, Color color);
void print_board_state_full(BoardState *board_s);

#endif