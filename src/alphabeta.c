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

const int MAX_SCORE = 100050;

int alpha_beta_score(PositionList *board_history, char color, int is_max)
{
    if ((is_max == 1 && color == 'w') || (is_max == 0 && color == 'b'))
    {
        return eval(board_history);
    }
    else
    {
        return -eval(board_history);
    }
}

MoveList *possible_moves(BoardState *board_s, char color)
{
    MoveList *move_list = malloc(sizeof(MoveList));
    if (move_list == NULL)
    {
        return NULL;
    }
    move_list->size = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Piece piece = board_s->board[i][j];
            if (piece.color == color && !is_empty(piece))
            {
                for (int k = 0; k < 8; k++)
                {
                    for (int l = 0; l < 8; l++)
                    {
                        Coords init_co = {i, j};
                        Coords dest_co = {k, l};
                        if (can_move_heuristic(board_s, piece, init_co, dest_co, true))
                        {
                            Move move = {init_co, dest_co, ' '};
                            if (piece.name == 'P' && (dest_co.x == 0 || dest_co.x == 7))
                            {
                                for (int p = 0; p < 4; p++)
                                {
                                    move.promotion = "QNBR"[p];
                                    move_list->moves[move_list->size] = move;
                                    move_list->size++;
                                }
                            }
                            else
                            {
                                move_list->moves[move_list->size] = move;
                                move_list->size++;
                            }
                        }
                    }
                }
            }
        }
    }
    return move_list;
}

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

MoveScore alphabeta(int alpha, int beta, int depth, int max_depth, PositionList *board_history, char color, Move tested_move, int is_max, int is_min, int *nodes, clock_t start_clk, double max_time, Move prio_move)
{
    *nodes = *nodes + 1;
    MoveScore result;
    result.move = tested_move;
    if (depth == max_depth)
    {
        result.score = alpha_beta_score(board_history, color, is_max);
        return result;
    }
    // MoveList *move_list2 = possible_moves(board_history->board_s, color);
    MoveList *move_list = possible_moves_bb(board_history->board_s);
    // verify_and_print_differences(move_list, move_list2, board_history, color);
    if (prio_move.init_co.x != -1)
    {
        move_list->moves[move_list->size] = prio_move;
        move_list->size++;
    }
    if (move_list->size == 0)
    {
        // if (is_check(board_history->board_s, color))
        if (is_king_in_check(board_history->board_s))
        {
            result.score = is_max ? -(MAX_SCORE - depth) : (MAX_SCORE - depth);
            free(move_list);
            return result;
        }
        result.score = 0;
        free(move_list);
        return result;
    }
    BoardState *new_board_s = malloc(sizeof(BoardState));
    PositionList *new_board_history = malloc(sizeof(PositionList));
    if (new_board_s == NULL || new_board_history == NULL)
    {
        result.score = 0;
        result.move = empty_move();
        free(move_list);
        return result;
    }
    new_board_history->tail = board_history;
    char next_color = color == 'w' ? 'b' : 'w';
    double time_taken;
    if (is_max)
    {
        result.score = -MAX_SCORE;
        for (int i = move_list->size - 1; i >= 0; i--)
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
            Move new_move = move_list->moves[i];
            *new_board_s = *board_history->board_s;
            new_board_s = move_piece(new_board_s, new_move);
            new_board_history->board_s = new_board_s;
            int new_score;
            bool eval_game_ended_res = eval_game_ended(new_board_history, &new_score);
            if (!eval_game_ended_res)
            {
                MoveScore new_move_score = alphabeta(alpha, beta, depth + 1, max_depth, new_board_history, next_color, new_move, 0, 1, nodes, start_clk, max_time, empty_move());
                new_score = new_move_score.score;
            }
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
                break;
            }
        }
    }
    else
    {
        result.score = MAX_SCORE;
        for (int i = move_list->size - 1; i >= 0; i--)
        {
            time_taken = ((double)(clock() - start_clk)) / CLOCKS_PER_SEC;
            if (time_taken > max_time)
            {
                // si on n'a pas fini d'évaler les coups de l'ennemi, on considère qu'il est dans une position gagnante
                result.score = -MAX_SCORE;
                break;
            }
            Move new_move = move_list->moves[i];
            *new_board_s = *board_history->board_s;
            new_board_s = move_piece(new_board_s, new_move);
            new_board_history->board_s = new_board_s;
            int new_score;
            bool eval_game_ended_res = eval_game_ended(new_board_history, &new_score);
            if (!eval_game_ended_res)
            {
                MoveScore new_move_score = alphabeta(alpha, beta, depth + 1, max_depth, new_board_history, next_color, new_move, 1, 0, nodes, start_clk, max_time, empty_move());
                new_score = new_move_score.score;
            }
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
                break;
            }
        }
    }
    free(move_list);
    free(new_board_s);
    free(new_board_history);
    return result;
}

// do an alpha beta iterative deepening search
// board_s is the current board state
// color is the color of the player to move
// max_depth is the maximum depth of the search
// return the best move found

Move iterative_deepening(PositionList *board_history, char color, int max_depth, double max_time)
{
    clock_t glob_start = clock();
    Move move = empty_move();
    MoveScore new_move_score;
    clock_t start_iter, end_iter;
    double cpu_time_used;
    int nodes = 0;
    int score;
    double nps;
    for (int i = 1; i <= max_depth; i++)
    {
        nodes = 0;
        start_iter = clock();
        new_move_score = alphabeta(-MAX_SCORE, MAX_SCORE, 0, i, board_history, color, empty_move(), 1, 0, &nodes, glob_start, max_time, move);
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
        if (score < -1000 && (total_time > max_time || i >= max_depth))
        {
            fprintf(stderr, "surrendering\n");
            return empty_move();
        }
        if (total_time > max_time)
        {
            break;
        }
    }
    return move;
}