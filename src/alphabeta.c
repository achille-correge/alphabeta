#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "types.h"
#include "chess_logic.h"
#include "eval.h"
#include "alphabeta.h"
#include "bitboards_moves.h"
#include "debug_functions.h"
#include "transposition_tables.h"
#include "alphabeta_auxiliary.h"

extern int quiesce_inodes;
extern int quiesce_nodes;

static int tt_hits = 0;
static int tt_cutoffs = 0;
static int nodes = 0;
static bool search_interrupted = false;

// do an optimized version of the possible moves function using bitboards
// board_s is the current board state
// color is the color of the player to move
// return the list of possible moves

typedef struct
{
    Move move;
    int score;
} MoveScore;

// do an alpha beta search

// alpha is the best score that the maximizing player can guarantee
// beta is the best score that the minimizing player can guarantee
// depth is the depth of the search
// max_depth is the maximum depth of the search
// board_s is the current board state
// color is the color of the player to move
// tested_move is the move to make
// nodes is the number of nodes checked
// return the score of the best move

MoveScore alphabeta(int alpha, int beta, int depth, int max_depth, TranspoTable *table, PositionList *board_history, Color color, Move tested_move, clock_t start_clk, double max_time, Move prio_move, Move killer_moves[2][MAX_SEARCH_PLY])
{
    nodes = nodes + 1;
    MoveScore result;
    result.move = tested_move;
    if ((threefold_hash(board_history->board_s->hash, board_history, 1) || board_history->board_s->fifty_move_rule >= 100) && depth > 0)
    {
        result.score = 0;
        return result;
    }
    if (depth >= max_depth)
    {
        // depth extension if in check (+14.0 +/- 3.4 elo)
        if (!(is_king_in_check(board_history->board_s) && depth-max_depth < 8))
        {
            // quiescence search
            result.score = quiesce(alpha, beta, depth, table, board_history, color);
            store_transposition_table_entry(table, board_history->board_s->hash, result.score, 0, empty_move(), EXACT);
            return result;
        }
    }
    MoveList move_list_val;
    MoveList *move_list = &move_list_val;
    init_possible_moves_bb(board_history->board_s, move_list);
    // Check for checks
    if (move_list->size == 0)
    {
        if (is_king_in_check(board_history->board_s))
        {
            result.score = -(MAX_SCORE - depth);
            return result;
        }
        result.score = 0;
        return result;
    }
    BoardState new_board_s_val;
    PositionList new_board_history_val;
    BoardState *new_board_s = &new_board_s_val;
    PositionList *new_board_history = &new_board_history_val;
    new_board_history->tail = board_history;
    Color next_color = color ^ 1;
    double time_taken;
    // Check transposition table
    int depth_to_go = max_depth - depth;
    if (depth_to_go < 0)
    {
        depth_to_go = 0;
    }
    Move tt_move = empty_move();
    int tt_depth = 0;
    if (tt_lookup(table, board_history->board_s->hash, depth_to_go, alpha, beta, &result.score, &tt_move, &tt_depth))
    {
        // Only use TT move if it's valid (to prevent hits on same hash entries with different positions)
        if (is_in_move_list(move_list, tt_move))
        {
            result.move = tt_move;
            // mate normalization
            // fprintf(stderr, "TT hit at depth %d, depth to go %d, with score %d\n", depth, depth_to_go, result.score);
            if (result.score > MAX_SCORE - 100)
            {
                result.score -= depth;
            }
            else if (result.score < -MAX_SCORE + 100)
            {
                result.score += depth;
            }
            tt_cutoffs++;
            return result;
        }
    }
    if (tt_depth > 0)
    {
        tt_hits++;
    }
    if (is_empty_move(prio_move) && tt_depth > 0)
    {
        prio_move = tt_move;
    }
    // Score moves for move ordering
    score_moves(board_history->board_s, move_list, prio_move, depth, killer_moves);
    Flag tt_flag = EXACT;
    result.score = -MAX_SCORE;
    for (int i = 0; i < move_list->size; i++)
    {
        if ((nodes << 25) == 0)
        {
            time_taken = ((double)(clock() - start_clk)) / CLOCKS_PER_SEC;
            if (time_taken > max_time)
            {
                // si on n'a pas fini d'évaluer nos coups, on prend le mieux qu'on a trouvé
                if (depth == 0)
                    {
                        fprintf(stderr, "time exceeded the limit, time taken: %f\n", time_taken);
                    }
                search_interrupted = true;
                return result;
            }
        }
        swap_best_move(move_list, i);
        Move new_move = move_list->moves[i];
        *new_board_s = *board_history->board_s;
        new_board_s = move_piece(new_board_s, new_move);
        new_board_history->board_s = new_board_s;
        int new_score;
        MoveScore new_move_score = alphabeta(-beta, -alpha, depth + 1, max_depth, table, new_board_history, next_color, new_move, start_clk, max_time, empty_move(), killer_moves);
        if (search_interrupted)
        {
            return result;
        }
        new_score = -new_move_score.score;
        if (new_score > result.score)
        {
            result.move = new_move;
            result.score = new_score;
        }
        if (result.score > alpha)
        {
            alpha = result.score;
        }
        if (alpha >= beta)
        {
            tt_flag = LOWERBOUND;
            insert_killer_move(killer_moves, new_move, depth);
            break;
        }
    }
    store_transposition_table_entry(table, board_history->board_s->hash, result.score, depth_to_go, result.move, tt_flag);
    return result;
}

// do an alpha beta iterative deepening search
// board_s is the current board state
// color is the color of the player to move
// max_depth is the maximum depth of the search
// return the best move found

Move iterative_deepening(TranspoTable *tt, PositionList *board_history, Color color, int max_depth, double max_time)
{
    clock_t glob_start = clock();
    Move move = empty_move();
    MoveScore new_move_score;
    clock_t start_iter, end_iter;
    double cpu_time_used;
    int score;
    // double nps;
    double total_nodes;
    double total_nps;
    Move killer_moves[2][MAX_SEARCH_PLY]; // initialize killer moves
    for (int k = 0; k < 2; k++)
    {
        for (int d = 0; d < MAX_SEARCH_PLY; d++)
        {
            killer_moves[k][d] = empty_move();
        }
    }
    for (int i = 1; i <= max_depth; i++)
    {
        nodes = 0;
        quiesce_nodes = 0;
        quiesce_inodes = 0;
        tt_hits = 0;
        tt_cutoffs = 0;
        search_interrupted = false;
        start_iter = clock();
        new_move_score = alphabeta(-MAX_SCORE, MAX_SCORE, 0, i, tt, board_history, color, empty_move(), glob_start, max_time, move, killer_moves);
        end_iter = clock();
        cpu_time_used = ((double)(end_iter - start_iter)) / CLOCKS_PER_SEC;
        // nps = nodes / cpu_time_used;
        total_nodes = nodes + quiesce_nodes;
        total_nps = total_nodes / cpu_time_used;
        if (!is_empty_move(new_move_score.move))
        {
            move = new_move_score.move;
        }
        double total_time = ((double)(clock() - glob_start)) / CLOCKS_PER_SEC;
        fprintf(stderr, "depth %d: move: %c%c -> %c%c, score: %d, time taken: %f\n", i, 'a' + move.init_co.y, '1' + move.init_co.x, 'a' + move.dest_co.y, '1' + move.dest_co.x, new_move_score.score, cpu_time_used);
        // fprintf(stderr, "         nodes checked: %d, quiesce nodes: %d, quiesce inodes: %d, nps: %f, total nps: %f, tt hits: %d, tt cutoffs: %d\n", nodes, quiesce_nodes, quiesce_inodes, nps, total_nps, tt_hits, tt_cutoffs);
        fprintf(stderr, "         ab nodes: %d, quiesce nodes: %d, total nps: %f\n", nodes, quiesce_nodes, total_nps);
        if (total_time > max_time && new_move_score.score < -10000)
            fprintf(stderr, "not a single move was evaluated on last iteration (hence -mate score), taking previous score as reference\n");
        else
            score = new_move_score.score;
        if (abs(score) >= MAX_SCORE - 50)
        {
            fprintf(stderr, "a mate was found\n");
            if(score > 0){
                break;
            }
        }
        // if (score < -1000 && (total_time > max_time || i >= max_depth))
        // {
        //     fprintf(stderr, "surrendering\n");
        //     return empty_move();
        // }
        if (total_time > max_time)
        {
            break;
        }
    }
    return move;
}