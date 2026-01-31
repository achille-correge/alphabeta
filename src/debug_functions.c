#include <stdio.h>
#include "types.h"
#include "chess_logic.h"
#include "debug_functions.h"

void print_bitboard(Bitboard b)
{
    for (int i = 63; i >= 0; i--)
    {
        fprintf(stderr, "%d", (int)(b >> i) & 1);
        if (i % 8 == 0)
        {
            fprintf(stderr, "\n");
        }
    }
    fprintf(stderr, "\n");
}

void print_move(Move move)
{
    char start_col = 'a' + move.init_co.y;
    char start_row = '1' + (move.init_co.x);
    char end_col = 'a' + move.dest_co.y;
    char end_row = '1' + (move.dest_co.x);
    fprintf(stderr, "%c%c -> %c%c\n", start_col, start_row, end_col, end_row);
}

void print_move_list(MoveList *move_list)
{
    fprintf(stderr, "size: %d\n", move_list->size);
    for (int i = 0; i < move_list->size; i++)
    {
        Move move = move_list->moves[i];
        fprintf(stderr, "init: %d %d, dest: %d %d\n", move.init_co.x, move.init_co.y, move.dest_co.x, move.dest_co.y);
    }
}

void print_board_debug(BoardState *board_s)
{
    Piece(*board)[8] = board_s->board;
    for (int i = 7; i >= 0; i--)
    {
        fprintf(stderr, "%d ", i);
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j].color == WHITE)
            {
                fprintf(stderr, "%c ", piece_type_to_char(board[i][j].name));
            }
            else if (board[i][j].color == BLACK)
            {
                fprintf(stderr, "%c ", piece_type_to_char(board[i][j].name) + 32);
            }
            else
            {
                fprintf(stderr, "  ");
            }
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "  0 1 2 3 4 5 6 7\n");
}

Bitboard get_targetbb_move_list(MoveList *move_list)
{
    Bitboard targetbb = 0ULL;
    for (int i = 0; i < move_list->size; i++)
    {
        Move move = move_list->moves[i];
        targetbb |= 1ULL << coords_to_square(move.dest_co);
    }
    return targetbb;
}

bool is_in_move_list(MoveList *move_list, Move move)
{
    for (int i = 0; i < move_list->size; i++)
    {
        if (move_list->moves[i].init_co.x == move.init_co.x &&
            move_list->moves[i].init_co.y == move.init_co.y &&
            move_list->moves[i].dest_co.x == move.dest_co.x &&
            move_list->moves[i].dest_co.y == move.dest_co.y &&
            move_list->moves[i].promotion == move.promotion)
        {
            return true;
        }
    }
    return false;
}

bool are_same_move_set(MoveList *move_list, MoveList *move_list_bb)
{
    if (move_list->size != move_list_bb->size)
    {
        return false;
    }
    for (int i = 0; i < move_list->size; i++)
    {
        if (!is_in_move_list(move_list_bb, move_list->moves[i]))
        {
            return false;
        }
    }
    return true;
}

void print_differences(MoveList *move_list, MoveList *move_list_bb)
{
    if (move_list->size > move_list_bb->size)
    {
        fprintf(stderr, "move_list has more moves\n");
        for (int i = 0; i < move_list->size; i++)
        {
            if (!is_in_move_list(move_list_bb, move_list->moves[i]))
            {
                Move move = move_list->moves[i];
                fprintf(stderr, "init: %d %d, dest: %d %d\n", move.init_co.x, move.init_co.y, move.dest_co.x, move.dest_co.y);
            }
        }
    }
    else
    {
        fprintf(stderr, "move_list_bb has more or same number of moves\n");
        for (int i = 0; i < move_list_bb->size; i++)
        {
            if (!is_in_move_list(move_list, move_list_bb->moves[i]))
            {
                Move move = move_list_bb->moves[i];
                fprintf(stderr, "init: %d %d, dest: %d %d\n", move.init_co.x, move.init_co.y, move.dest_co.x, move.dest_co.y);
            }
        }
    }
}

void verify_and_print_differences(MoveList *move_list, MoveList *move_list2, PositionList *board_history, Color color)
{
    if (!are_same_move_set(move_list, move_list2))
    {
        fprintf(stderr, "\n\nplayer: %d\n\n", color);
        fprintf(stderr, "move_list size: %d\n", move_list->size);
        print_bitboard(get_targetbb_move_list(move_list));
        // print_move_list(move_list);
        fprintf(stderr, "move_list1 size: %d\n", move_list2->size);
        print_bitboard(get_targetbb_move_list(move_list2));
        // print_move_list(move_list1);
        print_differences(move_list, move_list2);
        print_bitboard(get_targetbb_move_list(move_list) ^ get_targetbb_move_list(move_list2));
        // print_board_debug(board_history->board_s);
        print_board_state_full(board_history->board_s);
    }
}

void print_board_state_full(BoardState *board_s)
{
    print_board_debug(board_s);
    print_bitboard(board_s->color_bb[WHITE]);
    print_bitboard(board_s->color_bb[BLACK]);
    print_bitboard(board_s->all_pieces_bb[WHITE][PAWN]);
    print_bitboard(board_s->all_pieces_bb[BLACK][PAWN]);
    print_bitboard(board_s->all_pieces_bb[WHITE][KNIGHT]);
    print_bitboard(board_s->all_pieces_bb[BLACK][KNIGHT]);
    print_bitboard(board_s->all_pieces_bb[WHITE][BISHOP]);
    print_bitboard(board_s->all_pieces_bb[BLACK][BISHOP]);
    print_bitboard(board_s->all_pieces_bb[WHITE][ROOK]);
    print_bitboard(board_s->all_pieces_bb[BLACK][ROOK]);
    print_bitboard(board_s->all_pieces_bb[WHITE][QUEEN]);
    print_bitboard(board_s->all_pieces_bb[BLACK][QUEEN]);
    print_bitboard(board_s->all_pieces_bb[WHITE][KING]);
    print_bitboard(board_s->all_pieces_bb[BLACK][KING]);
    char color = board_s->player == WHITE ? 'w' : 'b';
    fprintf(stderr, "player: %c\n", color);
}