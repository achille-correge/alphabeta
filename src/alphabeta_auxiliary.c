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


static const int MVV_LVA[6] = {10, 30, 32, 50, 90, 95};  // values for pawn, knight, bishop, rook, queen, king

int alpha_beta_score(PositionList *board_history, Color color, int is_max)
{
    if ((is_max == 1 && color == WHITE) || (is_max == 0 && color == BLACK))
    {
        return eval(board_history);
    }
    else
    {
        return -eval(board_history);
    }
}

void insert_killer_move(Move killer_moves[2][MAX_SEARCH_PLY], Move move, int depth)
{
    // 1. Si le coup est déjà le premier killer, on ne fait rien
    if (moves_are_equal(killer_moves[0][depth], move)) {
        return;
    }

    // 2. Décalage (le premier devient le second)
    killer_moves[1][depth] = killer_moves[0][depth];
    killer_moves[0][depth] = move;
}

void score_move(BoardState *board_s, Move move, int *score, Move prio_move, int depth, Move killer_moves[2][MAX_SEARCH_PLY])
{
    // scale : 2M for PV move, 1M for captures and Q promo, 500k for killer moves, rest for quiet moves
    // PV move has highest priority
    *score = 0;
    if (moves_are_equal(prio_move, move))
    {
        *score += 2000000; // highest priority for the principal variation move
    }
    // CAPTURES and PROMOTIONS next
    Piece captured_piece = board_s->board[move.dest_co.x][move.dest_co.y];
    if (!is_empty(captured_piece))
    {
        int victim_value = MVV_LVA[captured_piece.name];
        int attacker_value = MVV_LVA[board_s->board[move.init_co.x][move.init_co.y].name];
        *score += (victim_value * 10 - attacker_value) + 1000000; // MVV-LVA scoring for captures
    }
    if (move.promotion != EMPTY_PIECE)
    {
        if (move.promotion == QUEEN)
        {
            *score+= 1000000; // extra bonus for promoting to queen
        }
        *score += MVV_LVA[move.promotion]; // bonus for promotion
    }
    // KILLER MOVES
    if (killer_moves != NULL) {
        if (moves_are_equal(killer_moves[0][depth], move))
        {
            *score += 500000;
        }
        else if (moves_are_equal(killer_moves[1][depth], move))
        {
            *score += 400000;
        }
    }
    // TO ADD : HISTORY HEURISTIC, ETC.
}

void score_moves(BoardState *board_s, MoveList *move_list, Move prio_move, int depth, Move killer_moves[2][MAX_SEARCH_PLY])
{
    for (int i = 0; i < move_list->size; i++)
    {
        score_move(board_s, move_list->moves[i], &move_list->moves_scores[i], prio_move, depth, killer_moves);
    }
}


void swap_best_move(MoveList *move_list, int index)
{
    int best_index = index;
    for (int j = index + 1; j < move_list->size; j++)
    {
        if (move_list->moves_scores[j] > move_list->moves_scores[best_index])
        {
            best_index = j;
        }
    }
    if (best_index != index)
    {
        // swap moves
        Move temp_move = move_list->moves[index];
        move_list->moves[index] = move_list->moves[best_index];
        move_list->moves[best_index] = temp_move;
        // swap scores
        int temp_score = move_list->moves_scores[index];
        move_list->moves_scores[index] = move_list->moves_scores[best_index];
        move_list->moves_scores[best_index] = temp_score;
    }
}


// Il manque la TT dans la quiescence search, mais ça fonctionne pas pareil
// (un move de quiescence ne doit pas remplacer un move normal dans la TT)
Score quiesce(int alpha, int beta, int depth, TranspoTable *table, PositionList *board_history, Color color, int is_max, int *nodes)
{
    // printf("QS Depth: %d, Nodes: %d\n", depth, *nodes);
    // 1. ÉVALUATION STATIQUE (Stand Pat)
    // C'est le score si on décidait d'arrêter les échanges ici.
    Score stand_pat = alpha_beta_score(board_history, color, is_max);

    if (is_max) {
        if (stand_pat >= beta) return stand_pat;
        if (stand_pat > alpha) alpha = stand_pat;
    } else {
        if (stand_pat <= alpha) return stand_pat;
        if (stand_pat < beta) beta = stand_pat;
    }

    Bitboard captures = get_attacks(board_history->board_s) & board_history->board_s->color_bb[color ^ 1];
    // fprintf(stderr, "QS Depth %d: Captures bitboard:\n", depth);
    if (captures == 0) {
        return stand_pat;
    }
    
    *nodes = *nodes + 1;
    // 2. GÉNÉRATION DES CAPTURES UNIQUEMENT
    MoveList capture_list_val;
    MoveList *capture_list = &capture_list_val;
    init_possible_capture_moves_bb(board_history->board_s, capture_list);
    
    // Tri des captures (MVV-LVA recommandé ici)
    score_moves(board_history->board_s, capture_list, empty_move(), depth, NULL);
    BoardState new_board_s_val;
    PositionList new_board_history_val;
    BoardState *new_board_s = &new_board_s_val;
    PositionList *new_board_history = &new_board_history_val;
    new_board_history->tail = board_history;

    // Score bestscore = is_max ? -MAX_SCORE : MAX_SCORE;
    Score bestscore = stand_pat;

    for (int i = 0; i < capture_list->size; i++) {

        swap_best_move(capture_list, i);
        Move new_move = capture_list->moves[i];

        // 3. DELTA PRUNING
        // Si même en capturant la pièce la plus chère (plus une marge), 
        // on n'atteint pas alpha, on ignore ce coup.
        if (is_max) {
            int capture_value = MVV_LVA[board_history->board_s->board[new_move.dest_co.x][new_move.dest_co.y].name];
            if (stand_pat + capture_value + 200 < alpha) continue;
        } else {
            int capture_value = MVV_LVA[board_history->board_s->board[new_move.dest_co.x][new_move.dest_co.y].name];
            if (stand_pat - capture_value - 200 > beta) continue;
        }

        // Exécution du coup

        *new_board_s = *board_history->board_s;
        new_board_s = move_piece(new_board_s, new_move);
        new_board_history->board_s = new_board_s;

        Score score = quiesce(alpha, beta, depth + 1, table, new_board_history, color ^ 1, !is_max, nodes);

        if (is_max) {
            if (score >= beta) return score;
            if (score > bestscore) bestscore = score;
            if (score > alpha) alpha = score;
        } else {
            if (score <= alpha) return score;
            if (score < bestscore) bestscore = score;
            if (score < beta) beta = score;
        }
    }

    return bestscore;
}