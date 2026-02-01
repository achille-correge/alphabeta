#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "chess_logic.h"
#include "types.h"
#include "bitboards_moves.h"
#include "debug_functions.h"
#include "transposition_tables.h"

const int PIECES_PHASE_VALUES[6] = {0, 1, 1, 2, 4, 0};

Piece empty_piece()
{
    Piece piece;
    piece.name = EMPTY_PIECE;
    piece.color = EMPTY_COLOR;
    return piece;
}

Coords empty_coords()
{
    Coords coords;
    coords.x = -1;
    coords.y = -1;
    return coords;
}

Move empty_move()
{
    Move move;
    move.init_co = empty_coords();
    move.dest_co = empty_coords();
    move.promotion = EMPTY_PIECE;
    return move;
}

bool is_empty(Piece piece)
{
    return piece.name == EMPTY_PIECE;
}

bool is_empty_coords(Coords coords)
{
    return coords.x == -1;
}

bool is_empty_coords_true(Coords coords)
{
    return coords.x == -1 && coords.y == -1;
}

bool is_empty_move(Move move)
{
    return move.init_co.x == -1;
}

bool is_empty_move_true(Move move)
{
    return is_empty_coords_true(move.init_co) && is_empty_coords_true(move.dest_co) && move.promotion == EMPTY_PIECE;
}

bool moves_are_equal(Move move1, Move move2)
{
    return (move1.init_co.x == move2.init_co.x &&
            move1.init_co.y == move2.init_co.y &&
            move1.dest_co.x == move2.dest_co.x &&
            move1.dest_co.y == move2.dest_co.y &&
            move1.promotion == move2.promotion);
}

int coords_to_square(Coords co)
{
    return co.x * 8 + 7 - co.y;
}

Coords square_to_coords(int square)
{
    Coords coords;
    coords.x = square / 8;
    coords.y = 7 - square % 8;
    return coords;
}

PieceType char_to_piece_type(char c)
{
    // Handle both uppercase and lowercase (UCI uses lowercase for promotions)
    if (c >= 'a' && c <= 'z')
        c = c - 'a' + 'A';
    
    switch (c)
    {
    case 'P':
        return PAWN;
    case 'N':
        return KNIGHT;
    case 'B':
        return BISHOP;
    case 'R':
        return ROOK;
    case 'Q':
        return QUEEN;
    case 'K':
        return KING;
    default:
        return EMPTY_PIECE;
    }
}

char piece_type_to_char(PieceType type)
{
    switch (type)
    {
    case PAWN:
        return 'P';
    case KNIGHT:
        return 'N';
    case BISHOP:
        return 'B';
    case ROOK:
        return 'R';
    case QUEEN:
        return 'Q';
    case KING:
        return 'K';
    case EMPTY_PIECE:
        return ' ';
    default:
        return ' ';
    }
}

PositionList *empty_list()
{
    return NULL;
}

void free_position_list(PositionList *pos_l)
{
    while (pos_l != NULL)
    {
        PositionList *temp = pos_l;
        pos_l = pos_l->tail;
        free(temp->board_s);
        free(temp);
    }
}

PositionList *save_position(BoardState *board_s, PositionList *pos_l)
{
    BoardState *board_s_copy = malloc(sizeof(BoardState));
    if (board_s_copy == NULL)
    {
        return NULL;
    }
    *board_s_copy = *board_s;
    PositionList *new_list = malloc(sizeof(PositionList));
    if (new_list == NULL)
    {
        return NULL;
    }
    new_list->board_s = board_s_copy;
    new_list->tail = pos_l;
    return new_list;
}

bool threefold_hash(uint64_t hash, PositionList *pos_l, int number_of_repetitions)
{
    if (pos_l == NULL)
    {
        return false;
    }
    else
    {
        if (hash == pos_l->board_s->hash)
        {
            number_of_repetitions++;
        }
        if (number_of_repetitions > 2)
        {
            return true;
        }
        return threefold_hash(hash, pos_l->tail, number_of_repetitions);
    }
}

bool insufficient_material(BoardState *board_s)
{
    int white_pieces = __builtin_popcountll(board_s->color_bb[WHITE]);
    int black_pieces = __builtin_popcountll(board_s->color_bb[BLACK]);
    int white_bishops_knight = __builtin_popcountll(board_s->all_pieces_bb[WHITE][BISHOP]) + __builtin_popcountll(board_s->all_pieces_bb[WHITE][KNIGHT]);
    int black_bishops_knight = __builtin_popcountll(board_s->all_pieces_bb[BLACK][BISHOP]) + __builtin_popcountll(board_s->all_pieces_bb[BLACK][KNIGHT]);
    if (white_pieces == 1 && black_pieces == 1)
    {
        return true;
    }
    else if (white_pieces == 2 && black_pieces == 1 && white_bishops_knight == 1)
    {
        return true;
    }
    else if (white_pieces == 1 && black_pieces == 2 && black_bishops_knight == 1)
    {
        return true;
    }
    else if (white_pieces == 2 && black_pieces == 2 && white_bishops_knight == 1 && black_bishops_knight == 1)
    {
        return true;
    }
    return false;
}

Piece get_piece(Piece board[8][8], Coords coords)
{
    if (is_empty_coords(coords))
    {
        return empty_piece();
    }
    return board[coords.x][coords.y];
}

BoardState *move_pawn_handling(BoardState *board_s, Piece move_piece, Piece dest_piece, Move sel_move)
{
    Coords new_coords = sel_move.dest_co;
    Coords init_coords = sel_move.init_co;

    // fprintf(stderr, "move_pawn_handling: color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
    // fprintf(stderr, "dest_piece: %c, %c\n", dest_piece.name, dest_piece.color);
    if ((move_piece.color == WHITE && new_coords.x == 7) || (move_piece.color == BLACK && new_coords.x == 0))
    {
        board_s->board[new_coords.x][new_coords.y].name = sel_move.promotion;
        board_s->all_pieces_bb[move_piece.color][sel_move.promotion] |= 1ULL << coords_to_square(new_coords);
        board_s->all_pieces_bb[move_piece.color][PAWN] &= ~(1ULL << coords_to_square(new_coords));
        board_s->phase += PIECES_PHASE_VALUES[sel_move.promotion];
    }
    if (move_piece.color == WHITE && new_coords.x - init_coords.x == 2)
    {
        // fprintf(stderr, "2 pas, color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
        board_s->white_pawn_passant = new_coords.y;
        board_s->hash ^= zobrist_table[772+ new_coords.y]; // add en passant square to the hash, 772 = white en passant
    }
    else if (move_piece.color == BLACK && init_coords.x - new_coords.x == 2)
    {
        board_s->black_pawn_passant = new_coords.y;
        board_s->hash ^= zobrist_table[780 + new_coords.y]; // add en passant square to the hash, 780 = black en passant
    }
    if (move_piece.color == WHITE && is_empty(dest_piece) && new_coords.y != init_coords.y)
    {
        board_s->board[new_coords.x - 1][new_coords.y] = empty_piece();
        board_s->color_bb[BLACK] &= ~(1ULL << (39 - new_coords.y)); // 39 = 4 * 8 + 7 (to get the fith row)
        board_s->all_pieces_bb[BLACK][PAWN] &= ~(1ULL << (39 - new_coords.y));
        board_s->hash ^= zobrist_table[6 * 64 + (39 - new_coords.y)]; // remove the captured pawn from the hash, 6 = black pawns
    }
    else if (move_piece.color == BLACK && is_empty(dest_piece) && new_coords.y != init_coords.y)
    {
        board_s->board[new_coords.x + 1][new_coords.y] = empty_piece();
        board_s->color_bb[WHITE] &= ~(1ULL << (31 - new_coords.y)); // 31 = 3 * 8 + 7 (to get the second row)
        board_s->all_pieces_bb[WHITE][PAWN] &= ~(1ULL << (31 - new_coords.y));
        board_s->hash ^= zobrist_table[31 - new_coords.y]; // remove the captured pawn from the hash, 0 = white pawns
    }
    return board_s;
}

BoardState *move_king_handling(BoardState *board_s, Piece piece, Coords init_coords, Coords new_coords)
{
    if (piece.color == WHITE)
    {
        board_s->white_kingside_castlable = false;
        board_s->white_queenside_castlable = false;
        board_s->hash ^= zobrist_table[768]; // remove white kingside castling right from the hash, 768 = white kingside
        board_s->hash ^= zobrist_table[769]; // remove white queenside castling right from the hash, 769 = white queenside
    }
    else
    {
        board_s->black_kingside_castlable = false;
        board_s->black_queenside_castlable = false;
        board_s->hash ^= zobrist_table[770]; // remove black kingside castling right from the hash, 770 = black kingside
        board_s->hash ^= zobrist_table[771]; // remove black queenside castling right from the hash, 771 = black queenside
    }
    if (new_coords.y == 6 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][5].name = ROOK;
        board_s->board[new_coords.x][5].color = piece.color;
        board_s->board[new_coords.x][7] = empty_piece();
        board_s->color_bb[piece.color] ^= 1UL << (8 * new_coords.x);
        board_s->color_bb[piece.color] |= 1UL << (8 * new_coords.x + 2);
        board_s->all_pieces_bb[piece.color][ROOK] ^= 1UL << (8 * new_coords.x);
        board_s->all_pieces_bb[piece.color][ROOK] |= 1UL << (8 * new_coords.x + 2);
        board_s->hash ^= zobrist_table[(ROOK+6*piece.color) * 64 + 8 * new_coords.x + 7];     // remove rook from original square
        board_s->hash ^= zobrist_table[(ROOK+6*piece.color) * 64 + 8 * new_coords.x + 5];     // add rook to new square
    }
    else if (new_coords.y == 2 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][3].name = ROOK;
        board_s->board[new_coords.x][3].color = piece.color;
        board_s->board[new_coords.x][0] = empty_piece();
        board_s->color_bb[piece.color] ^= 1UL << (8 * new_coords.x + 7);
        board_s->color_bb[piece.color] |= 1UL << (8 * new_coords.x + 4);
        board_s->all_pieces_bb[piece.color][ROOK] ^= 1UL << (8 * new_coords.x + 7);
        board_s->all_pieces_bb[piece.color][ROOK] |= 1UL << (8 * new_coords.x + 4);
        board_s->hash ^= zobrist_table[(ROOK+6*piece.color) * 64 + 8 * new_coords.x + 0];     // remove rook from original square
        board_s->hash ^= zobrist_table[(ROOK+6*piece.color) * 64 + 8 * new_coords.x + 3];     // add rook to new square
    }
    return board_s;
}

BoardState *move_rook_handling(BoardState *board_s, Piece piece, Coords init_coords, Coords new_coords)
{
    if (piece.color == WHITE && init_coords.x == 0 && init_coords.y == 0)
    {
        board_s->white_queenside_castlable = false;
        board_s->hash ^= zobrist_table[769]; // remove white queenside castling right from the hash, 769 = white queenside
    }
    else if (piece.color == WHITE && init_coords.x == 0 && init_coords.y == 7)
    {
        board_s->white_kingside_castlable = false;
        board_s->hash ^= zobrist_table[768]; // remove white kingside castling right from the hash, 768 = white kingside
    }
    else if (piece.color == BLACK && init_coords.x == 7 && init_coords.y == 0)
    {
        board_s->black_queenside_castlable = false;
        board_s->hash ^= zobrist_table[771]; // remove black queenside castling right from the hash, 771 = black queenside
    }
    else if (piece.color == BLACK && init_coords.x == 7 && init_coords.y == 7)
    {
        board_s->black_kingside_castlable = false;
        board_s->hash ^= zobrist_table[770]; // remove black kingside castling right from the hash, 770 = black kingside
    }
    return board_s;
}

BoardState *move_piece(BoardState *board_s, Move sel_move)
{
    Coords init_coords = sel_move.init_co;
    Coords new_coords = sel_move.dest_co;
    Piece move_piece = get_piece(board_s->board, init_coords);
    Piece dest_piece = get_piece(board_s->board, new_coords);
    Color color = move_piece.color;
    Color enemy_color = color ^ 1;
    if (is_empty(move_piece))
    {
        return board_s;
    }
    // fprintf(stderr, "move_piece: color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
    // update board
    if (board_s->white_pawn_passant != -1)
    {
        board_s->hash ^= zobrist_table[772 + board_s->white_pawn_passant]; // remove en passant square from the hash, 772 = white en passant
        board_s->white_pawn_passant = -1;
    }
    if (board_s->black_pawn_passant != -1)
    {
        board_s->hash ^= zobrist_table[780 + board_s->black_pawn_passant]; // remove en passant square from the hash, 780 = black en passant
        board_s->black_pawn_passant = -1;
    }
    // put the piece in the new location
    board_s->board[new_coords.x][new_coords.y].name = move_piece.name;
    board_s->board[new_coords.x][new_coords.y].color = move_piece.color;
    board_s->color_bb[color] |= 1ULL << coords_to_square(new_coords);
    board_s->all_pieces_bb[color][move_piece.name] |= 1ULL << coords_to_square(new_coords);
    board_s->hash ^= zobrist_table[(move_piece.name + 6 * color) * 64 + 8*new_coords.x + new_coords.y]; // add the moved piece to the hash
    // remove the piece from the old location
    board_s->board[init_coords.x][init_coords.y] = empty_piece();
    board_s->color_bb[color] ^= 1ULL << coords_to_square(init_coords);
    board_s->all_pieces_bb[color][move_piece.name] ^= 1ULL << coords_to_square(init_coords);
    board_s->hash ^= zobrist_table[(move_piece.name + 6 * color) * 64 + 8*init_coords.x + init_coords.y]; // remove the moved piece from the hash
    // remove the piece from the enemy if it exists
    if (!is_empty(dest_piece))
    {
        board_s->color_bb[enemy_color] ^= 1ULL << coords_to_square(new_coords);
        board_s->all_pieces_bb[enemy_color][dest_piece.name] ^= 1ULL << coords_to_square(new_coords);
        board_s->phase -= PIECES_PHASE_VALUES[dest_piece.name];
        board_s->hash ^= zobrist_table[(dest_piece.name + 6 * enemy_color) * 64 + 8*new_coords.x + new_coords.y]; // remove the captured piece from the hash
    }
    // fifty move rule
    if (is_empty(dest_piece) && move_piece.name != PAWN)
    {
        board_s->fifty_move_rule++;
    }
    else
    {
        board_s->fifty_move_rule = 0;
    }
    // handle special moves
    if (move_piece.name == PAWN)
    {
        board_s = move_pawn_handling(board_s, move_piece, dest_piece, sel_move);
    }
    else if (move_piece.name == KING)
    {
        board_s = move_king_handling(board_s, move_piece, init_coords, new_coords);
    }
    else if (move_piece.name == ROOK)
    {
        board_s = move_rook_handling(board_s, move_piece, init_coords, new_coords);
    }
    // switch player
    board_s->player = 1 - board_s->player;
    board_s->hash ^= zobrist_table[780]; // switch player in hash, 780 = side to move
    return board_s;
}

BoardState *init_board()
{
    BoardState *board_s = malloc(sizeof(BoardState));
    if (board_s == NULL)
    {
        return NULL;
    }
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            board_s->board[i][j].name = EMPTY_PIECE;
            board_s->board[i][j].color = EMPTY_COLOR;
        }
    }
    for (int i = 0; i < 8; i++)
    {
        board_s->board[1][i].name = PAWN;
        board_s->board[6][i].name = PAWN;
        board_s->board[0][i].color = WHITE;
        board_s->board[1][i].color = WHITE;
        board_s->board[6][i].color = BLACK;
        board_s->board[7][i].color = BLACK;
    }
    for (int i = 0; i < 8; i = i + 7)
    {
        board_s->board[i][0].name = ROOK;
        board_s->board[i][7].name = ROOK;
        board_s->board[i][1].name = KNIGHT;
        board_s->board[i][6].name = KNIGHT;
        board_s->board[i][2].name = BISHOP;
        board_s->board[i][5].name = BISHOP;
        board_s->board[i][3].name = QUEEN;
        board_s->board[i][4].name = KING;
    }
    board_s->white_kingside_castlable = true;
    board_s->white_queenside_castlable = true;
    board_s->black_kingside_castlable = true;
    board_s->black_queenside_castlable = true;
    board_s->black_pawn_passant = -1;
    board_s->white_pawn_passant = -1;
    board_s->fifty_move_rule = 0;

    board_s->player = WHITE;
    board_s->color_bb[WHITE] = init_white();
    board_s->color_bb[BLACK] = init_black();
    board_s->all_pieces_bb[WHITE][PAWN] = init_white_pawns();
    board_s->all_pieces_bb[BLACK][PAWN] = init_black_pawns();
    board_s->all_pieces_bb[WHITE][KNIGHT] = init_white_knights();
    board_s->all_pieces_bb[BLACK][KNIGHT] = init_black_knights();
    board_s->all_pieces_bb[WHITE][BISHOP] = init_white_bishops();
    board_s->all_pieces_bb[BLACK][BISHOP] = init_black_bishops();
    board_s->all_pieces_bb[WHITE][ROOK] = init_white_rooks();
    board_s->all_pieces_bb[BLACK][ROOK] = init_black_rooks();
    board_s->all_pieces_bb[WHITE][QUEEN] = init_white_queens();
    board_s->all_pieces_bb[BLACK][QUEEN] = init_black_queens();
    board_s->all_pieces_bb[WHITE][KING] = init_white_kings();
    board_s->all_pieces_bb[BLACK][KING] = init_black_kings();

    board_s->phase = 24; // starting phase (all pieces except kings)
    board_s->hash = get_zobrist_hash(board_s);

    return board_s;
}

int compute_phase(BoardState *board_s)
{
    int phase = 0;
    for (int piece_type = PAWN; piece_type <= QUEEN; piece_type++)
    {
        int white_count = __builtin_popcountll(board_s->all_pieces_bb[WHITE][piece_type]);
        int black_count = __builtin_popcountll(board_s->all_pieces_bb[BLACK][piece_type]);
        phase += PIECES_PHASE_VALUES[piece_type] * (white_count + black_count);
    }
    return phase;
}

BoardState *FEN_to_board(char *FEN)
{
    BoardState *board_s = init_board();
    // initialize all bitboards to 0
    board_s->color_bb[WHITE] = 0;
    board_s->color_bb[BLACK] = 0;
    for (int i = 0; i < 6; i++)
    {
        board_s->all_pieces_bb[WHITE][i] = 0;
        board_s->all_pieces_bb[BLACK][i] = 0;
    }
    if (board_s == NULL)
    {
        return NULL;
    }
    int i = 0;
    int xx = 0;
    int y = 0;
    while (FEN[i] != ' ')
    {
        if (FEN[i] == '/')
        {
            xx++;
            y = 0;
        }
        else if (FEN[i] >= '1' && FEN[i] <= '8')
        {
            for (int k = 0; k < FEN[i] - '0'; k++)
            {
                board_s->board[7 - xx][y].name = EMPTY_PIECE;
                board_s->board[7 - xx][y].color = EMPTY_COLOR;
                y++;
            }
        }
        else
        {
            if (FEN[i] >= 'A' && FEN[i] <= 'Z')
            {
                board_s->board[7 - xx][y].color = WHITE;
                board_s->board[7 - xx][y].name = char_to_piece_type(FEN[i]);
                board_s->color_bb[WHITE] |= 1ULL << (8 * (7 - xx) + 7 - y);
                board_s->all_pieces_bb[WHITE][char_to_piece_type(FEN[i])] |= 1ULL << (8 * (7 - xx) + 7 - y);
            }
            else
            {
                board_s->board[7 - xx][y].color = BLACK;
                board_s->board[7 - xx][y].name = char_to_piece_type(FEN[i] - 'a' + 'A');
                board_s->color_bb[BLACK] |= 1ULL << (8 * (7 - xx) + 7 - y);
                board_s->all_pieces_bb[BLACK][char_to_piece_type(FEN[i] - 'a' + 'A')] |= 1ULL << (8 * (7 - xx) + 7 - y);
            }
            y++;
        }
        i++;
    }
    i++;
    if (FEN[i] == 'w' || 'b')
    {
        board_s->player = FEN[i] == 'w' ? WHITE : BLACK;
        i = i + 2;
    }
    board_s->white_kingside_castlable = false;
    board_s->white_queenside_castlable = false;
    board_s->black_kingside_castlable = false;
    board_s->black_queenside_castlable = false;
    if (FEN[i] == '-')
    {
        i++;
    }
    else
    {
        while (FEN[i] != ' ')
        {
            if (FEN[i] == 'K')
            {
                board_s->white_kingside_castlable = true;
            }
            else if (FEN[i] == 'Q')
            {
                board_s->white_queenside_castlable = true;
            }
            else if (FEN[i] == 'k')
            {
                board_s->black_kingside_castlable = true;
            }
            else if (FEN[i] == 'q')
            {
                board_s->black_queenside_castlable = true;
            }
            i++;
        }
    }
    i++;
    if (FEN[i] != '-')
    {
        board_s->black_pawn_passant = FEN[i] - 'a';
        board_s->white_pawn_passant = FEN[i] - 'a';
    }
    else
    {
        board_s->black_pawn_passant = -1;
        board_s->white_pawn_passant = -1;
    }
    i = i + 2;
    board_s->fifty_move_rule = FEN[i] - '0';
    board_s->phase = compute_phase(board_s);
    board_s->hash = get_zobrist_hash(board_s);
    return board_s;
}
