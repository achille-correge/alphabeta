#ifndef ALPHABETA_AUXILIARY_H
#define ALPHABETA_AUXILIARY_H
#include "types.h"

int alpha_beta_score(PositionList *board_history, Color color, int is_max);
void insert_killer_move(Move killer_moves[2][MAX_SEARCH_PLY], Move move, int depth);
void score_moves(BoardState *board_s, MoveList *move_list, Move prio_move, int depth, Move killer_moves[2][MAX_SEARCH_PLY]);
void swap_best_move(MoveList *move_list, int index);
Score quiesce(SearchContext *ctx, int alpha, int beta, int depth, PositionList *board_history);

#endif
