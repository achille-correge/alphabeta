#include <stdlib.h>

#include "types.h"
#include "transposition_tables.h"
#include "chess_logic.h"

uint64_t get_zobrist_hash(BoardState *board_s)
{
    uint64_t hash = 0;
    Piece(*board)[8] = board_s->board;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Piece piece = board[i][j];
            if (!is_empty(piece))
            {
                int piece_index = piece.name + piece.color * 6;
                hash ^= zobrist_table[piece_index * 64 + i * 8 + j];
            }
        }
    }
    if (board_s->white_kingside_castlable)
        hash ^= zobrist_table[768];
    if (board_s->white_queenside_castlable)
        hash ^= zobrist_table[769];
    if (board_s->black_kingside_castlable)
        hash ^= zobrist_table[770];
    if (board_s->black_queenside_castlable)
        hash ^= zobrist_table[771];
    if (board_s->black_pawn_passant != -1)
        hash ^= zobrist_table[772 + board_s->black_pawn_passant];
    if (board_s->white_pawn_passant != -1)
        hash ^= zobrist_table[772 + board_s->white_pawn_passant];
    if (board_s->player == BLACK)
        hash ^= zobrist_table[780];
    return hash;
}

void initialize_transposition_table(TranspoTable *table, size_t size)
{
    table->size = size;
    table->entries = (TranspoTableEntry *)calloc(size, sizeof(TranspoTableEntry));
}

void free_transposition_table(TranspoTable *table)
{
    free(table->entries);
    table->entries = NULL;
    table->size = 0;
}

size_t get_transposition_table_index(TranspoTable *table, uint64_t hash)
{
    return hash % table->size;
}

TranspoTableEntry *get_transposition_table_entry(TranspoTable *table, uint64_t hash)
{
    size_t index = get_transposition_table_index(table, hash);
    return &table->entries[index];
}

void store_transposition_table_entry(TranspoTable *table, uint64_t hash, Score score, int depth, Move best_move, Flag flag)
{
    size_t index = get_transposition_table_index(table, hash);
    TranspoTableEntry *entry = &table->entries[index];
    entry->hash = hash;
    entry->score = score;
    entry->depth = depth;
    entry->best_move = best_move;
    entry->flag = flag;
}

bool tt_lookup(TranspoTable *table, uint64_t hash, int depth_to_go, int alpha, int beta, int *score, Move *best_move, int *tt_depth) {
    TranspoTableEntry *entry = get_transposition_table_entry(table, hash);

    if (entry->hash == hash && entry->depth >= depth_to_go && entry->score != 0) {
        // Avoid using entries with zero score (could be polluted by contexts like threefold repetition)
        if (entry->flag == EXACT) {
            *score = entry->score;
            *best_move = entry->best_move;
            *tt_depth = entry->depth;
            return true;
        }
        if (entry->flag == LOWERBOUND && entry->score >= beta) {
            *score = entry->score;
            *best_move = entry->best_move;
            *tt_depth = entry->depth;
            return true;
        }
        if (entry->flag == UPPERBOUND && entry->score <= alpha) {
            *score = entry->score;
            *best_move = entry->best_move;
            *tt_depth = entry->depth;
            return true;
        }
    }
    return false;
}