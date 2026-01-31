#ifndef ALPHABETA_H
#define ALPHABETA_H

#include "chess_logic.h"
#include "types.h"
#include "eval.h"

MoveList *possible_moves(BoardState *board_s, Color color);
Move iterative_deepening(TranspoTable *tt, PositionList *board_history, Color color, int max_depth, double max_time);

#endif