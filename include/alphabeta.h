#ifndef ALPHABETA_H
#define ALPHABETA_H

#include "chess_logic.h"
#include "types.h"
#include "eval.h"

typedef struct {
    int max_depth;
    TranspoTable *table;
    clock_t start_clk;
    double max_time;
    int nodes;
    int tt_hits;
    int tt_cutoffs;
    int quiesce_nodes;
    int quiesce_inodes;
    bool search_interrupted;
    Move killer_moves[2][MAX_SEARCH_PLY];
} SearchContext;

Move iterative_deepening(TranspoTable *tt, PositionList *board_history, Color color, int max_depth, double max_time);

#endif