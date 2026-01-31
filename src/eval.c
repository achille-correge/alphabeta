#include "eval.h"
#include "types.h"
#include "chess_logic.h"
#include <stdlib.h>
#include <stdbool.h>

// Using PeSTO's piece-square tables and values

const int PIECES_VALUES_MG[6] = {82, 337, 365, 477, 1025, 0};
const int PIECES_VALUES_EG[6] = {94, 281, 297, 512, 936, 0};

const int passed_pawn_bonus[8] = {0, 15, 25, 35, 50, 80, 120, 0};

const int KING_TABLE_MG[8][8] = {
    {-65,  23,  16, -15, -56, -34,   2,  13},
    { 29,  -1, -20,  -7,  -8,  -4, -38, -29},
    { -9,  24,   2, -16, -20,   6,  22, -22},
    {-17, -20, -12, -27, -30, -25, -14, -36},
    {-49,  -1, -27, -39, -46, -44, -33, -51},
    {-14, -14, -22, -46, -44, -30, -15, -27},
    {  1,   7,  -8, -64, -43, -16,   9,   8},
    {-15,  36,  12, -54,   8, -28,  24,  14},
};

const int KING_TABLE_EG[8][8] = {
    {-74, -35, -18, -18, -11,  15,   4, -17},
    {-12,  17,  14,  17,  17,  38,  23,  11},
    { 10,  17,  23,  15,  20,  45,  44,  13},
    { -8,  22,  24,  27,  26,  33,  26,   3},
    {-18,  -4,  21,  24,  27,  23,   9, -11},
    {-19,  -3,  11,  21,  23,  16,   7,  -9},
    {-27, -11,   4,  13,  14,   4,  -5, -17},
    {-53, -34, -21, -11, -28, -14, -24, -43},
};

const int PAWN_TABLE_MG[8][8] = {
    { 0,   0,   0,   0,   0,   0,  0,   0},
    {98, 134,  61,  95,  68, 126, 34, -11},
    {-6,   7,  26,  31,  65,  56, 25, -20},
    {-14,  13,   6,  21,  23,  12, 17, -23},
    {-27,  -2,  -5,  12,  17,   6, 10, -25},
    {-26,  -4,  -4, -10,   3,   3, 33, -12},
    {-35,  -1, -20, -23, -15,  24, 38, -22},
    { 0,   0,   0,   0,   0,   0,  0,   0},
};

const int PAWN_TABLE_EG[8][8] = {
    { 0,   0,   0,   0,   0,   0,   0,   0},
    {178, 173, 158, 134, 147, 132, 165, 187},
    { 94, 100,  85,  67,  56,  53,  82,  84},
    { 32,  24,  13,   5,  -2,   4,  17,  17},
    { 13,   9,  -3,  -7,  -7,  -8,   3,  -1},
    { 4,   7,  -6,   1,   0,  -5,  -1,  -8},
    { 13,   8,   8,  10,  13,   0,   2,  -7},
    { 0,   0,   0,   0,   0,   0,   0,   0},
};

const int KNIGHT_TABLE_MG[8][8] = {
    {-167, -89, -34, -49,  61, -97, -15, -107},
    { -73, -41,  72,  36,  23,  62,   7,  -17},
    { -47,  60,  37,  65,  84, 129,  73,   44},
    {  -9,  17,  19,  53,  37,  69,  18,   22},
    { -13,   4,  16,  13,  28,  19,  21,   -8},
    { -23,  -9,  12,  10,  19,  17,  25,  -16},
    { -29, -53, -12,  -3,  -1,  18, -14,  -19},
    {-105, -21, -58, -33, -17, -28, -19,  -23},
};

const int KNIGHT_TABLE_EG[8][8] = {
    {-58, -38, -13, -28, -31, -27, -63, -99},
    {-25,  -8, -25,  -2,  -9, -25, -24, -52},
    {-24, -20,  10,   9,  -1,  -9, -19, -41},
    {-17,   3,  22,  22,  22,  11,   8, -18},
    {-18,  -6,  16,  25,  16,  17,   4, -18},
    {-23,  -3,  -1,  15,  10,  -3, -20, -22},
    {-42, -20, -10,  -5,  -2, -20, -23, -44},
    {-29, -51, -23, -15, -22, -18, -50, -64},
};

const int BISHOP_TABLE_MG[8][8] = {
    {-29,   4, -82, -37, -25, -42,   7,  -8},
    {-26,  16, -18, -13,  30,  59,  18, -47},
    {-16,  37,  43,  40,  35,  50,  37,  -2},
    { -4,   5,  19,  50,  37,  37,   7,  -2},
    { -6,  13,  13,  26,  34,  12,  10,   4},
    { 0,  15,  15,  15,  14,  27,  18,  10},
    { 4,  15,  16,   0,   7,  21,  33,   1},
    {-33,  -3, -14, -21, -13, -12, -39, -21},
};

const int BISHOP_TABLE_EG[8][8] = {
    {-14, -21, -11,  -8, -7,  -9, -17, -24},
    { -8,  -4,   7, -12, -3, -13,  -4, -14},
    { 2,  -8,   0,  -1, -2,   6,   0,   4},
    {-3,   9,  12,   9, 14,  10,   3,   2},
    {-6,   3,  13,  19,  7,  10,  -3,  -9},
    {-12,  -3,   8,  10, 13,   3,  -7, -15},
    {-14, -18,  -7,  -1,  4,  -9, -15, -27},
    {-23,  -9, -23,  -5, -9, -16,  -5, -17},
};

const int ROOK_TABLE_MG[8][8] = {
    { 32,  42,  32,  51, 63,  9,  31,  43},
    { 27,  32,  58,  62, 80, 67,  26,  44},
    { -5,  19,  26,  36, 17, 45,  61,  16},
    {-24, -11,   7,  26, 24, 35,  -8, -20},
    {-36, -26, -12,  -1,  9, -7,   6, -23},
    {-45, -25, -16, -17,  3,  0,  -5, -33},
    {-44, -16, -20,  -9, -1, 11,  -6, -71},
    {-19, -13,   1,  17, 16,  7, -37, -26},
};

const int ROOK_TABLE_EG[8][8] = {
    {13, 10, 18, 15, 12,  12,   8,   5},
    {11, 13, 13, 11, -3,   3,   8,   3},
    { 7,  7,  7,  5,  4,  -3,  -5,  -3},
    { 4,  3, 13,  1,  2,   1,  -1,   2},
    { 3,  5,  8,  4, -5,  -6,  -8, -11},
    {-4,  0, -5, -1, -7, -12,  -8, -16},
    {-6, -6,  0,  2, -9,  -9, -11,  -3},
    {-9,  2,  3, -1, -5, -13,   4, -20},
};

const int QUEEN_TABLE_MG[8][8] = {
    {-28,   0,  29,  12,  59,  44,  43,  45},
    {-24, -39,  -5,   1, -16,  57,  28,  54},
    {-13, -17,   7,   8,  29,  56,  47,  57},
    {-27, -27, -16, -16,  -1,  17,  -2,   1},
    { -9, -26,  -9, -10,  -2,  -4,   3,  -3},
    {-14,   2, -11,  -2,  -5,   2,  14,   5},
    {-35,  -8,  11,   2,   8,  15,  -3,   1},
    { -1, -18,  -9,  10, -15, -25, -31, -50},
};

const int QUEEN_TABLE_EG[8][8] = {
    { -9,  22,  22,  27,  27,  19,  10,  20},
    {-17,  20,  32,  41,  58,  25,  30,   0},
    {-20,   6,   9,  49,  47,  35,  19,   9},
    { 3,  22,  24,  45,  57,  40,  57,  36},
    {-18,  28,  19,  47,  31,  34,  39,  23},
    {-16, -27,  15,   6,   9,  17,  10,   5},
    {-22, -23, -30, -16, -16, -23, -36, -32},
    {-33, -28, -22, -43,  -5, -32, -20, -41},
};

const int (*PESTO_TABLE_MG[6])[8] = {
    PAWN_TABLE_MG,
    KNIGHT_TABLE_MG,
    BISHOP_TABLE_MG,
    ROOK_TABLE_MG,
    QUEEN_TABLE_MG,
    KING_TABLE_MG
};

const int (*PESTO_TABLE_EG[6])[8] = {
    PAWN_TABLE_EG,
    KNIGHT_TABLE_EG,
    BISHOP_TABLE_EG,
    ROOK_TABLE_EG,
    QUEEN_TABLE_EG,
    KING_TABLE_EG
};

int mg_table[2][6][8][8];
int eg_table[2][6][8][8];

void init_eval_tables()
{
    for (int color = 0; color < 2; color++)
    {
        for (int piece = 0; piece < 6; piece++)
        {
            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    if (color == WHITE)
                    {
                        mg_table[color][piece][i][j] = PESTO_TABLE_MG[piece][i][j] + PIECES_VALUES_MG[piece];
                        eg_table[color][piece][i][j] = PESTO_TABLE_EG[piece][i][j] + PIECES_VALUES_EG[piece];
                    }
                    else
                    {
                        mg_table[color][piece][i][j] = PESTO_TABLE_MG[piece][7 - i][j] + PIECES_VALUES_MG[piece];
                        eg_table[color][piece][i][j] = PESTO_TABLE_EG[piece][7 - i][j] + PIECES_VALUES_EG[piece];
                    }
                }
            }
        }
    }
}

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
#define FILE_C 0x2020202020202020
#define FILE_D 0x1010101010101010
#define FILE_E 0x0808080808080808
#define FILE_F 0x0404040404040404
#define FILE_G 0x0202020202020202
#define FILE_H 0x0101010101010101

// masks to get all the ranks under or over a certain rank
const Bitboard under_over_ranks[2][8] = {
    {0x0000000000000000, 0x00000000000000FF, 0x000000000000FFFF, 0x0000000000FFFFFF, 0x00000000FFFFFFFF, 0x000000FFFFFFFFFF, 0x0000FFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF},
    {0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFF000000, 0xFFFFFFFF00000000, 0xFFFFFF0000000000, 0xFFFF000000000000, 0xFF00000000000000, 0x0000000000000000}};

// evaluate pawn structure
int one_pawn_structure_eval(BoardState *board_s, int x, int y, Color color)
{
    int score = 0;
    Bitboard pawns = board_s->all_pieces_bb[color][PAWN];
    Bitboard opponent_pawns = board_s->all_pieces_bb[color ^ 1][PAWN];
    Bitboard in_front_mask = under_over_ranks[color ^ 1][x];
    Bitboard left_file = FILE_H << (7 - y + 1);
    Bitboard pwn_file = FILE_H << (7-y);
    Bitboard right_file = FILE_H << (7-y-1);
    Bitboard pwn_row = RANK_1 << x*8;
    // Bitboard pwn = pwn_file & pwn_row;
    // int start_row = color == 'w' ? 6 : 1;
    // bool semi_open_file = (pwn_file & opponent_pawns) == 0;

    // ISOLATED
    if ((pawns & left_file) == 0 && (pawns & right_file) == 0)
    {
        score -= 15;
    }

    // BACKWARD
    // if ((pawns & in_front_mask & left_file & right_file) != 0 && x != start_row)
    // {
    //     if (semi_open_file)
    //     {
    //         score -= 10;
    //     }
    // }

    // DOUBLED
    if ((pwn_file & pawns & ~pwn_row) != 0)
    {
        score -= 5;
    }

    // PASSED
    if ((opponent_pawns & left_file & right_file & in_front_mask) == 0)
    {
        int advancement = color == WHITE ? x : (7 - x);
        score += passed_pawn_bonus[advancement];
    }

    // PROTECTED or PROTECTS
    if ((pawns & (left_file | right_file) & (pwn_row << 8 | pwn_row >> 8)) != 0)
    {
        score += 10;
    }
    return score;
}

// Loop over all the pawns and evaluate their structure
int pawn_structure_eval(BoardState *board_s)
{
    int score = 0;
    Bitboard white_pawns = board_s->all_pieces_bb[WHITE][PAWN];
    Bitboard black_pawns = board_s->all_pieces_bb[BLACK][PAWN];
    while (white_pawns)
    {
        int square = __builtin_ctzll(white_pawns);
        int x = square / 8;
        int y = square % 8;
        score += one_pawn_structure_eval(board_s, x, y, WHITE);
        white_pawns &= white_pawns - 1;
    }
    while (black_pawns)
    {
        int square = __builtin_ctzll(black_pawns);
        int x = square / 8;
        int y = square % 8;
        score -= one_pawn_structure_eval(board_s, x, y, BLACK);
        black_pawns &= black_pawns - 1;
    }
    return score;
}


bool check_for_mates(BoardState *board_s, Color color, int *result)
{
    bool white_mate = is_mate(board_s, WHITE);
    bool black_mate = is_mate(board_s, BLACK);
    if (white_mate && is_check(board_s, WHITE))
    {
        *result = -MAX_SCORE;
    }
    else if (black_mate && is_check(board_s, BLACK))
    {
        *result = MAX_SCORE;
    }
    else if (white_mate || black_mate)
    {
        *result = 0;
    }
    else
    {
        return false;
    }
    return true;
}

bool eval_game_ended(PositionList *board_history, int *result)
{
    BoardState *board_s = board_history->board_s;

    // check for mates, not necessary in the way we call this function
    // if (check_for_mates(board_s, 'w', result) || check_for_mates(board_s, 'b', result))
    // {
    //     return true;
    // }
    // else
    if (threefold_repetition(board_s, board_history, 0))
    {
        // printf("threefold repetition\n");
        *result = -100;
    }
    else if (board_s->fifty_move_rule >= 100)
    {
        *result = -50;
    }
    else if (insufficient_material(board_s))
    {
        *result = 0;
    }
    else
    {
        return false;
    }
    return true;
}

// evaluate the board state for the white player
// return the score of the board state
int pieces_eval(PositionList *board_history)
{
    BoardState *board_s = board_history->board_s;
    Piece(*board)[8] = board_s->board;
    int phase = board_s->phase;
    // printf("eval\n");
    // evaluate the board
    int pieces_eval_mg = 0;
    int pieces_eval_eg = 0;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Piece piece = board[i][j];
            if (piece.color == WHITE)
            {
                pieces_eval_mg += mg_table[WHITE][piece.name][i][j];
                pieces_eval_eg += eg_table[WHITE][piece.name][i][j];
            }
            else if (piece.color == BLACK)
            {
                pieces_eval_mg -= mg_table[BLACK][piece.name][i][j];
                pieces_eval_eg -= eg_table[BLACK][piece.name][i][j];
            }
        }
    }
    int mgphase = phase;
    if (mgphase > 24)
        mgphase = 24;
    int egphase = 24 - mgphase;
    // fprintf(stderr, "Pieces eval mg: %d, eg: %d, phase: %d\n", pieces_eval_mg, pieces_eval_eg, phase);
    return (pieces_eval_mg * mgphase + pieces_eval_eg * egphase) / 24;
}

int castle_eval(BoardState *board_s)
{
    // grère l'éval du castling (avoir l'option de castle est toujours un avantage
    // mais surtout si on l'a pas fait, il ne faut pas bouger le roi au centre)
    // Stockfish faisait pas comme ça bien sûr, y a moyen de rework sur la sécurité du roi
    // Auquel cas, plus faire appel aux phases, lissage etc serait de bon ton
    int score = 0;
    if (board_s->white_kingside_castlable || board_s->white_queenside_castlable)
    {
        score += 10;
    }
    else if ((board_s->all_pieces_bb[WHITE][KING] & 0x0000000000003838) && board_s->phase > 13)
    {
        score -= 50;
    }
    if (board_s->black_kingside_castlable || board_s->black_queenside_castlable)
    {
        score -= 10;
    }
    else if ((board_s->all_pieces_bb[BLACK][KING] & 0x3838000000000000) && board_s->phase > 13)
    {
        score += 50;
    }
    return score;
}

int eval(PositionList *board_history)
{
    int score = pieces_eval(board_history);
    // fprintf(stderr, "Pieces eval: %d\n", score);
    // Pawn structure eval + castle eval : Elo difference: 16.0 +/- 9.5, LOS: 100.0 %, DrawRatio: 61.2 %
    score += pawn_structure_eval(board_history->board_s);    
    score += castle_eval(board_history->board_s);            
    return score;
}