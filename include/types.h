#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_SEARCH_PLY 64
#define MAX_MOVES 512
#define MAX_SCORE 32000

typedef int Score;

typedef uint64_t Bitboard;

typedef enum : uint8_t
{
    WHITE,
    BLACK,
    EMPTY_COLOR
} Color;

typedef enum : uint8_t
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    EMPTY_PIECE
} PieceType;

typedef struct
{
    PieceType name;  // name of the piece
    Color color; // color of the piece ('w' for white, 'b' for black, ' ' for empty)
} Piece;

typedef struct
{
    int8_t x;
    int8_t y;
} Coords;

typedef struct
{
    Coords init_co;
    Coords dest_co;
    PieceType promotion;
} Move;

typedef struct
{
    Piece board[8][8];
    bool white_kingside_castlable; // true if white can castle
    bool white_queenside_castlable;
    bool black_kingside_castlable;
    bool black_queenside_castlable;
    int black_pawn_passant; // -1 if no pawn can be taken en passant, otherwise the column of the pawn
    int white_pawn_passant;
    int fifty_move_rule;
    Color player;

    // redondance ou pour calculer rapidement ou pour l'éval
    Bitboard color_bb[2];
    Bitboard all_pieces_bb[2][6];
    uint64_t hash;
    int phase;
} BoardState;

typedef struct position_list
{
    BoardState *board_s;
    struct position_list *tail;
} PositionList;

typedef struct
{
    Move moves[MAX_MOVES];
    int moves_scores[MAX_MOVES];
    int size;
} MoveList;

typedef enum : uint8_t
{
    EXACT,
    LOWERBOUND,
    UPPERBOUND
} Flag;

typedef struct
{
    uint64_t hash;
    Score score;
    uint8_t depth;
    Flag flag;
    Move best_move;
} TranspoTableEntry;

typedef struct
{
    size_t size;
    TranspoTableEntry *entries;
} TranspoTable;

#endif