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
// is_max is true if the current player is the maximizing player
// is_min is true if the current player is the minimizing player
// nodes is the number of nodes checked
// return the score of the best move

MoveScore alphabeta(int alpha, int beta, int depth, int max_depth, TranspoTable *table, PositionList *board_history, Color color, Move tested_move, int is_max, int is_min, int *nodes, clock_t start_clk, double max_time, Move prio_move, Move killer_moves[2][MAX_SEARCH_PLY])
{
    *nodes = *nodes + 1;
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
            result.score = quiesce(alpha, beta, depth, table, board_history, color, is_max, nodes);
            // store_transposition_table_entry(table, board_history->board_s->hash, result.score, 0, empty_move(), EXACT);
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
            result.score = is_max ? -(MAX_SCORE - depth) : (MAX_SCORE - depth);
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
    if (tt_lookup(table, board_history->board_s->hash, depth_to_go, alpha, beta, &result.score, &tt_move))
    {
        // Only use TT move if it's valid (to prevent hits on same hash entries with different positions)
        if (is_in_move_list(move_list, tt_move))
        {
            result.move = tt_move;
            // mate normalization
            if (result.score > MAX_SCORE - 100)
            {
                result.score -= depth;
            }
            else if (result.score < -MAX_SCORE + 100)
            {
                result.score += depth;
            }
            return result;
        }
    }
    if (is_empty_move(prio_move) && !is_empty_move(tt_move))
    {
        prio_move = tt_move;
    }
    // Score moves for move ordering
    score_moves(board_history->board_s, move_list, prio_move, depth, killer_moves);
    Flag tt_flag = EXACT;
    if (is_max)
    {
        result.score = -MAX_SCORE;
        for (int i = 0; i < move_list->size; i++)
        {
            time_taken = ((double)(clock() - start_clk)) / CLOCKS_PER_SEC;
            if (time_taken > max_time)
            {
                // si on n'a pas fini d'évaluer nos coups, on prend le mieux qu'on a trouvé
                if (depth == 0)
                    {
                        fprintf(stderr, "time exceeded the limit, time taken: %f\n", time_taken);
                    }
                break;
            }
            swap_best_move(move_list, i);
            Move new_move = move_list->moves[i];
            *new_board_s = *board_history->board_s;
            new_board_s = move_piece(new_board_s, new_move);
            new_board_history->board_s = new_board_s;
            int new_score;
            MoveScore new_move_score = alphabeta(alpha, beta, depth + 1, max_depth, table, new_board_history, next_color, new_move, 0, 1, nodes, start_clk, max_time, empty_move(), killer_moves);
            new_score = new_move_score.score;
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
                tt_flag = UPPERBOUND;
                insert_killer_move(killer_moves, new_move, depth);
                break;
            }
        }
    }
    else
    {
        result.score = MAX_SCORE;
        for (int i = 0; i < move_list->size; i++)
        {
            time_taken = ((double)(clock() - start_clk)) / CLOCKS_PER_SEC;
            if (time_taken > max_time)
            {
                // si on n'a pas fini d'évaler les coups de l'ennemi, on considère qu'il est dans une position gagnante
                result.score = -MAX_SCORE;
                break;
            }
            swap_best_move(move_list, i);
            Move new_move = move_list->moves[i];
            *new_board_s = *board_history->board_s;
            new_board_s = move_piece(new_board_s, new_move);
            new_board_history->board_s = new_board_s;
            int new_score;
            MoveScore new_move_score = alphabeta(alpha, beta, depth + 1, max_depth, table, new_board_history, next_color, new_move, 1, 0, nodes, start_clk, max_time, empty_move(), killer_moves);
            new_score = new_move_score.score;
            if (new_score < result.score)
            {
                result.score = new_score;
                result.move = new_move;
            }
            if (result.score < beta)
            {
                beta = result.score;
            }
            if (alpha >= beta)
            {
                tt_flag = LOWERBOUND;
                insert_killer_move(killer_moves, new_move, depth);
                break;
            }
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
    int nodes = 0;
    int score;
    double nps;
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
        start_iter = clock();
        new_move_score = alphabeta(-MAX_SCORE, MAX_SCORE, 0, i, tt, board_history, color, empty_move(), 1, 0, &nodes, glob_start, max_time, move, killer_moves);
        end_iter = clock();
        cpu_time_used = ((double)(end_iter - start_iter)) / CLOCKS_PER_SEC;
        nps = nodes / cpu_time_used;
        if (!is_empty_move(new_move_score.move))
        {
            move = new_move_score.move;
        }
        double total_time = ((double)(clock() - glob_start)) / CLOCKS_PER_SEC;
        fprintf(stderr, "depth: %d, move: %c%c -> %c%c, score: %d, time taken: %f, nodes checked: %d, nps: %f\n", i, 'a' + move.init_co.y, '1' + move.init_co.x, 'a' + move.dest_co.y, '1' + move.dest_co.x, new_move_score.score, cpu_time_used, nodes, nps);
        if (total_time > max_time && new_move_score.score < -1000)
            fprintf(stderr, "no move was completed on last iteration, taking previous score as reference\n");
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