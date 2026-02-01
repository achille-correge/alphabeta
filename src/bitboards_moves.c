#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "types.h"
#include "magic_tables.h"
#include "debug_functions.h"
#include "chess_logic.h"

// use bitboards to generate possible moves

typedef uint64_t Bitboard;

#define RANK_1 0xFF
#define RANK_2 0xFF00
#define RANK_3 0xFF0000
#define RANK_4 0xFF000000
#define RANK_5 0xFF00000000
#define RANK_6 0xFF0000000000
#define RANK_7 0xFF000000000000
#define RANK_8 0xFF00000000000000
#define FILE_A 0x8080808080808080
#define FILE_B 0x4040404040404040
#define FILE_G 0x0202020202020202
#define FILE_H 0x0101010101010101

Bitboard init_white()
{
    return 0xFFFF;
}

Bitboard init_black()
{
    return 0xFFFF000000000000;
}

Bitboard get_empty(Bitboard white, Bitboard black)
{
    return ~(white | black);
}

Bitboard init_white_pawns()
{
    return 0xFF00;
}

Bitboard init_black_pawns()
{
    return 0xFF000000000000;
}

// the not files are used to avoid wrap arounds
Bitboard get_white_pawn_pseudo_moves(Bitboard pawns, Bitboard empty, Bitboard enemy, int en_passant)
{
    Bitboard moves_1_square = (pawns << 8) & empty;
    Bitboard moves_2_squares = ((moves_1_square & RANK_3) << 8) & empty;
    Bitboard attacks = ((pawns & ~FILE_A) << 9) & enemy;
    attacks |= ((pawns & ~FILE_H) << 7) & enemy;
    if (en_passant != -1)
    {
        Bitboard en_passant_square = 1ULL << (47 - en_passant); // 7 - en_passant + 5*8 is the square of the en passant pawn
        attacks |= (pawns & ~FILE_A) << 9 & en_passant_square;
        attacks |= (pawns & ~FILE_H) << 7 & en_passant_square;
    }
    return (moves_1_square | moves_2_squares | attacks);
}

Bitboard get_black_pawn_pseudo_moves(Bitboard pawns, Bitboard empty, Bitboard enemy, int en_passant)
{
    Bitboard moves_1_square = (pawns >> 8) & empty;
    Bitboard moves_2_squares = ((moves_1_square & RANK_6) >> 8) & empty;
    Bitboard attacks = ((pawns & ~FILE_H) >> 9) & enemy;
    attacks |= ((pawns & ~FILE_A) >> 7) & enemy;
    if (en_passant != -1)
    {
        Bitboard en_passant_square = 1ULL << (23 - en_passant); // 7 - en_passant + 2*8
        attacks |= (pawns & ~FILE_H) >> 9 & en_passant_square;
        attacks |= (pawns & ~FILE_A) >> 7 & en_passant_square;
    }
    return (moves_1_square | moves_2_squares | attacks);
}

Bitboard init_white_knights()
{
    return 0x42;
}

Bitboard init_black_knights()
{
    return 0x4200000000000000;
}

Bitboard get_knight_pseudo_moves(Bitboard knights, Bitboard ally)
{
    Bitboard knight_moves = 0;
    Bitboard not_a_file = ~FILE_A;
    Bitboard not_ab_file = ~FILE_A & ~FILE_B;
    Bitboard not_h_file = ~FILE_H;
    Bitboard not_gh_file = ~FILE_H & ~FILE_G;
    Bitboard knight_moves_north = (knights << 17) & not_h_file;
    knight_moves_north |= (knights << 15) & not_a_file;
    knight_moves_north |= (knights << 10) & not_gh_file;
    knight_moves_north |= (knights << 6) & not_ab_file;
    Bitboard knight_moves_south = (knights >> 17) & not_a_file;
    knight_moves_south |= (knights >> 15) & not_h_file;
    knight_moves_south |= (knights >> 10) & not_ab_file;
    knight_moves_south |= (knights >> 6) & not_gh_file;
    knight_moves = knight_moves_north | knight_moves_south;
    return knight_moves & ~ally;
}

Bitboard init_white_bishops()
{
    return 0x24;
}

Bitboard init_black_bishops()
{
    return 0x2400000000000000;
}

Bitboard get_bishop_moves_square(Bitboard blockers, int square)
{
    blockers &= bishop_entry_masks[square];
    Bitboard index = (blockers * bishop_magic_numbers[square]) >> (64 - bishop_index_bits[square]);
    return bishop_tables[square][index];
}

Bitboard get_bishop_pseudo_moves(Bitboard bishops, Bitboard ally, Bitboard blockers)
{
    Bitboard bishop_moves = 0;
    while (bishops)
    {
        int square = __builtin_ctzll(bishops); // Get the index of the least significant bit set to 1. What a banger!!
        bishops &= bishops - 1;                // Reset the least significant bit set to 1
        bishop_moves |= get_bishop_moves_square(blockers, square);
    }
    return bishop_moves & ~ally;
}

Bitboard init_white_rooks()
{
    return 0x81;
}

Bitboard init_black_rooks()
{
    return 0x8100000000000000;
}

Bitboard get_rook_moves_square(Bitboard blockers, int square)
{
    blockers &= rook_entry_masks[square];
    Bitboard index = (blockers * rook_magic_numbers[square]) >> (64 - rook_index_bits[square]);
    return rook_table[square][index];
}

Bitboard get_rook_pseudo_moves(Bitboard rooks, Bitboard ally, Bitboard blockers)
{
    Bitboard rook_moves = 0;
    while (rooks)
    {
        int square = __builtin_ctzll(rooks);
        rooks &= rooks - 1;
        rook_moves |= get_rook_moves_square(blockers, square);
    }
    return rook_moves & ~ally;
}

Bitboard init_white_queens()
{
    return 0x10;
}

Bitboard init_black_queens()
{
    return 0x1000000000000000;
}

Bitboard get_queen_pseudo_moves(Bitboard queens, Bitboard ally, Bitboard blockers)
{
    return get_rook_pseudo_moves(queens, ally, blockers) | get_bishop_pseudo_moves(queens, ally, blockers);
}

Bitboard init_white_kings()
{
    return 0x8;
}

Bitboard init_black_kings()
{
    return 0x0800000000000000;
}

Bitboard get_king_pseudo_moves_nocastle(Bitboard kings, Bitboard ally)
{
    Bitboard not_a_file = ~FILE_A;
    Bitboard not_h_file = ~FILE_H;
    Bitboard king_moves = ((kings << 8) | (kings >> 8) |
                           ((kings << 1) & not_h_file) | ((kings >> 1) & not_a_file) |
                           ((kings << 9) & not_h_file) | ((kings << 7) & not_a_file) |
                           ((kings >> 9) & not_a_file) | ((kings >> 7) & not_h_file));
    return king_moves & ~ally;
}

Bitboard get_king_pseudo_moves(Bitboard kings, Bitboard ally, Bitboard blockers, Bitboard threatened_squares, Color color, bool kingside_castlable, bool queenside_castlable)
{
    Bitboard not_a_file = ~FILE_A;
    Bitboard not_h_file = ~FILE_H;
    Bitboard king_moves = ((kings << 8) | (kings >> 8) |
                           ((kings << 1) & not_h_file) | ((kings >> 1) & not_a_file) |
                           ((kings << 9) & not_h_file) | ((kings << 7) & not_a_file) |
                           ((kings >> 9) & not_a_file) | ((kings >> 7) & not_h_file));

    if (kingside_castlable)
    {
        Bitboard kingside_block_mask = color == WHITE ? 6 : 0x0600000000000000;
        Bitboard kingside_threatened_mask = color == WHITE ? 0xe : 0x0e00000000000000;
        if ((blockers & kingside_block_mask) == 0 && (threatened_squares & kingside_threatened_mask) == 0)
            king_moves |= (kings >> 2);
    }
    if (queenside_castlable)
    {
        Bitboard queenside_block_mask = color == WHITE ? 0x70 : 0x7000000000000000;
        Bitboard queenside_threatened_mask = color == WHITE ? 0x38 : 0x3800000000000000;
        if ((blockers & queenside_block_mask) == 0 && (threatened_squares & queenside_threatened_mask) == 0)
        {
            king_moves |= (kings << 2);
        }
    }

    return king_moves & ~ally;
}

Bitboard get_attacks(BoardState *board_s)
{
    Color color = board_s->player;
    Color enemy_color = color ^ 1;
    Bitboard ally = board_s->color_bb[color];
    Bitboard enemy = board_s->color_bb[enemy_color];
    Bitboard blockers = ally | enemy;
    Bitboard(*all_pieces_bb)[6] = board_s->all_pieces_bb;
    Bitboard attacks = 0;
    if (enemy_color == WHITE)
        attacks |= get_white_pawn_pseudo_moves(all_pieces_bb[enemy_color][PAWN], ~ally & ~enemy, ally, board_s->black_pawn_passant);
    else
        attacks |= get_black_pawn_pseudo_moves(all_pieces_bb[enemy_color][PAWN], ~ally & ~enemy, ally, board_s->white_pawn_passant);
    attacks |= get_knight_pseudo_moves(all_pieces_bb[enemy_color][KNIGHT], enemy);
    attacks |= get_rook_pseudo_moves(all_pieces_bb[enemy_color][ROOK], enemy, blockers);
    attacks |= get_bishop_pseudo_moves(all_pieces_bb[enemy_color][BISHOP], enemy, blockers);
    attacks |= get_queen_pseudo_moves(all_pieces_bb[enemy_color][QUEEN], enemy, blockers);
    attacks |= get_king_pseudo_moves_nocastle(all_pieces_bb[enemy_color][KING], enemy);
    return attacks;
}

bool is_king_in_check(BoardState *board_s)
{
    // in theory we shouldn't need to check all the (pseudo) attacks, en passant rule has no effect on the the king for example
    // but it's easier to just get all the attacks, hopefully this will not really affect performance
    Bitboard kings;
    kings = board_s->all_pieces_bb[board_s->player][KING];
    Bitboard attacks = get_attacks(board_s);
    return (attacks & kings) != 0;
}

Bitboard get_new_attacks(Bitboard all_pieces_bb[2][6], Bitboard enemy, Bitboard blockers, Color enemy_color)
{
    Bitboard attacks = 0;
    attacks |= get_rook_pseudo_moves(all_pieces_bb[enemy_color][ROOK], enemy, blockers);
    attacks |= get_bishop_pseudo_moves(all_pieces_bb[enemy_color][BISHOP], enemy, blockers);
    attacks |= get_queen_pseudo_moves(all_pieces_bb[enemy_color][QUEEN], enemy, blockers);
    return attacks;
}

bool is_king_left_in_check(Bitboard all_pieces_bb[2][6], Bitboard enemy, Bitboard blockers, Color color)
{
    Bitboard king = all_pieces_bb[color][KING];
    Bitboard attacks = get_new_attacks(all_pieces_bb, enemy, blockers, color ^ 1);
    return (attacks & king) != 0;
}

void add_move_co(MoveList *move_list, int init_square, int dest_square, PieceType piece_type)
{
    if (piece_type == PAWN && (dest_square / 8 == 0 || dest_square / 8 == 7))
    {
        PieceType promotions[] = {QUEEN, KNIGHT, BISHOP, ROOK};
        for (int p = 0; p < 4; p++)
        {
            move_list->moves[move_list->size].init_co = square_to_coords(init_square);
            move_list->moves[move_list->size].dest_co = square_to_coords(dest_square);
            move_list->moves[move_list->size].promotion = promotions[p];
            move_list->size++;
        }
    }
    else
    {
        move_list->moves[move_list->size].init_co = square_to_coords(init_square);
        move_list->moves[move_list->size].dest_co = square_to_coords(dest_square);
        move_list->moves[move_list->size].promotion = EMPTY_PIECE;
        move_list->size++;
    }
}

Bitboard get_single_piece_legal_moves(Bitboard piece, Bitboard piece_moves, BoardState *board_s, PieceType piece_type, bool is_check, MoveList *move_list)
{
    int piece_square = __builtin_ctzll(piece);
    Bitboard legal_moves = 0;
    Color color = board_s->player;
    Color enemy_color = color ^ 1;
    Bitboard ally = board_s->color_bb[color];
    Bitboard enemy = board_s->color_bb[enemy_color];
    Bitboard move = 0;

    Bitboard blockers = ally | enemy;
    Bitboard blockers_temp;
    Bitboard all_pieces_bb_temp[2][6];

    BoardState board_s_temp_val;
    BoardState *board_s_temp = &board_s_temp_val;
    if (piece_type == KING)
    {
        piece_moves &= ~get_attacks(board_s);
    }
    while (piece_moves)
    {
        int move_square = __builtin_ctzll(piece_moves);
        piece_moves &= piece_moves - 1;
        move = 1ULL << move_square;

        // if the king is in check we need to check if the move removes the check
        // if it is the king that moves we need to check if it will not be threatened after (more than only the already threatened squares)
        // In these cases we do the complete move at it is more easy
        if (is_check || piece_type == KING)
        {
            // Make the move on a temporary board state
            *board_s_temp = *board_s;
            board_s_temp->all_pieces_bb[color][piece_type] = (board_s->all_pieces_bb[color][piece_type] & ~piece) | move;
            board_s_temp->color_bb[color] = (board_s->color_bb[color] & ~piece) | move;
            for (PieceType enemy_piece_type = PAWN; enemy_piece_type <= KING; enemy_piece_type++)
            {
                board_s_temp->all_pieces_bb[enemy_color][enemy_piece_type] &= ~move;
            }
            board_s_temp->color_bb[enemy_color] &= ~move;
            if (!is_king_in_check(board_s_temp))
            {
                legal_moves |= move;
                add_move_co(move_list, piece_square, move_square, piece_type);
            }
        }
        else // else we only need to check if the move doesn't put the king in check
        {
            // Make the move temporarily only for the blockers and enemies
            memcpy(all_pieces_bb_temp, board_s->all_pieces_bb, sizeof(all_pieces_bb_temp));
            for (PieceType enemy_piece_type = PAWN; enemy_piece_type <= KING; enemy_piece_type++)
            {
                all_pieces_bb_temp[enemy_color][enemy_piece_type] = board_s->all_pieces_bb[enemy_color][enemy_piece_type] & ~move;
            }
            blockers_temp = (blockers & ~piece) | move;
            if (!is_king_left_in_check(all_pieces_bb_temp, enemy, blockers_temp, color))
            {
                legal_moves |= move;
                add_move_co(move_list, piece_square, move_square, piece_type);
            }
        }
    }
    return legal_moves;
}

Bitboard get_piece_moves(BoardState *board_s, PieceType piece_type, bool is_check, MoveList *move_list)
{
    Bitboard pieces = board_s->all_pieces_bb[board_s->player][piece_type];
    Bitboard legal_moves = 0;
    while (pieces)
    {
        int piece_square = __builtin_ctzll(pieces);
        pieces &= pieces - 1;
        Bitboard piece = 1ULL << piece_square;
        Bitboard piece_moves;
        switch (piece_type)
        {
        case PAWN:
            if (board_s->player == WHITE)
                piece_moves = get_white_pawn_pseudo_moves(piece, ~board_s->color_bb[WHITE] & ~board_s->color_bb[BLACK], board_s->color_bb[BLACK], board_s->black_pawn_passant);
            else
                piece_moves = get_black_pawn_pseudo_moves(piece, ~board_s->color_bb[WHITE] & ~board_s->color_bb[BLACK], board_s->color_bb[WHITE], board_s->white_pawn_passant);
            break;
        case KNIGHT:
            piece_moves = get_knight_pseudo_moves(piece, board_s->color_bb[board_s->player]);
            break;
        case BISHOP:
            piece_moves = get_bishop_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case ROOK:
            piece_moves = get_rook_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case QUEEN:
            piece_moves = get_queen_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case KING:
            if (board_s->player == WHITE)
                piece_moves = get_king_pseudo_moves(piece, board_s->color_bb[WHITE], board_s->color_bb[WHITE] | board_s->color_bb[BLACK], get_attacks(board_s), WHITE, board_s->white_kingside_castlable, board_s->white_queenside_castlable);
            else
                piece_moves = get_king_pseudo_moves(piece, board_s->color_bb[BLACK], board_s->color_bb[WHITE] | board_s->color_bb[BLACK], get_attacks(board_s), BLACK, board_s->black_kingside_castlable, board_s->black_queenside_castlable);
            break;
        default:
            piece_moves = 0;
            break;
        }
        legal_moves |= get_single_piece_legal_moves(piece, piece_moves, board_s, piece_type, is_check, move_list);
    }
    return legal_moves;
}

Bitboard get_piece_capture_moves(BoardState *board_s, PieceType piece_type, bool is_check, MoveList *capture_move_list)
{
    Bitboard pieces = board_s->all_pieces_bb[board_s->player][piece_type];
    Bitboard legal_moves = 0;
    while (pieces)
    {
        int piece_square = __builtin_ctzll(pieces);
        pieces &= pieces - 1;
        Bitboard piece = 1ULL << piece_square;
        Bitboard piece_moves;
        switch (piece_type)
        {
        case PAWN:
            if (board_s->player == WHITE)
                piece_moves = get_white_pawn_pseudo_moves(piece, ~board_s->color_bb[WHITE] & ~board_s->color_bb[BLACK], board_s->color_bb[BLACK], board_s->black_pawn_passant);
            else
                piece_moves = get_black_pawn_pseudo_moves(piece, ~board_s->color_bb[WHITE] & ~board_s->color_bb[BLACK], board_s->color_bb[WHITE], board_s->white_pawn_passant);
            break;
        case KNIGHT:
            piece_moves = get_knight_pseudo_moves(piece, board_s->color_bb[board_s->player]);
            break;
        case BISHOP:
            piece_moves = get_bishop_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case ROOK:
            piece_moves = get_rook_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case QUEEN:
            piece_moves = get_queen_pseudo_moves(piece, board_s->color_bb[board_s->player], board_s->color_bb[WHITE] | board_s->color_bb[BLACK]);
            break;
        case KING:
            if (board_s->player == WHITE)
                piece_moves = get_king_pseudo_moves(piece, board_s->color_bb[WHITE], board_s->color_bb[WHITE] | board_s->color_bb[BLACK], get_attacks(board_s), WHITE, board_s->white_kingside_castlable, board_s->white_queenside_castlable);
            else
                piece_moves = get_king_pseudo_moves(piece, board_s->color_bb[BLACK], board_s->color_bb[WHITE] | board_s->color_bb[BLACK], get_attacks(board_s), BLACK, board_s->black_kingside_castlable, board_s->black_queenside_castlable);
            break;
        default:
            piece_moves = 0;
            break;
        }
        piece_moves &= board_s->color_bb[board_s->player ^ 1];
        legal_moves |= get_single_piece_legal_moves(piece, piece_moves, board_s, piece_type, is_check, capture_move_list);
    }
    return legal_moves;
}

void init_possible_moves_bb(BoardState *board_s, MoveList *move_list)
{
    bool is_check = is_king_in_check(board_s);
    move_list->size = 0;
    get_piece_moves(board_s, PAWN, is_check, move_list);
    get_piece_moves(board_s, KNIGHT, is_check, move_list);
    get_piece_moves(board_s, BISHOP, is_check, move_list);
    get_piece_moves(board_s, ROOK, is_check, move_list);
    get_piece_moves(board_s, QUEEN, is_check, move_list);
    get_piece_moves(board_s, KING, is_check, move_list);
}

void init_possible_capture_moves_bb(BoardState *board_s, MoveList *move_capture_list)
{
    bool is_check = is_king_in_check(board_s);
    move_capture_list->size = 0;
    get_piece_capture_moves(board_s, PAWN, is_check, move_capture_list);
    get_piece_capture_moves(board_s, KNIGHT, is_check, move_capture_list);
    get_piece_capture_moves(board_s, BISHOP, is_check, move_capture_list);
    get_piece_capture_moves(board_s, ROOK, is_check, move_capture_list);
    get_piece_capture_moves(board_s, QUEEN, is_check, move_capture_list);
}

bool is_mate_bb(BoardState *board_s)
{
    MoveList move_list_val;
    MoveList *move_list = &move_list_val;
    init_possible_moves_bb(board_s, move_list);
    bool result = (move_list->size == 0);
    return result;
}
