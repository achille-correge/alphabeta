#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "chess_logic.h"
#include "types.h"
#include "legal_moves.h"
#include "chess_logic.h"
#include "bitboards_moves.h"
#include "debug_functions.h"

const int PIECES_PHASE_VALUES[6] = {0, 1, 1, 2, 4, 0};

Piece empty_piece()
{
    Piece piece;
    piece.name = ' ';
    piece.color = ' ';
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
    move.promotion = ' ';
    return move;
}

bool is_empty(Piece piece)
{
    return piece.name == ' ' && piece.color == ' ';
}

bool is_empty_coords(Coords coords)
{
    return coords.x == -1 && coords.y == -1;
}

bool is_empty_move(Move move)
{
    return is_empty_coords(move.init_co) && is_empty_coords(move.dest_co) && move.promotion == ' ';
}

bool are_same_piece(Piece piece1, Piece piece2)
{
    return piece1.name == piece2.name && piece1.color == piece2.color;
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

Color char_to_color(char c)
{
    return c == 'w' ? WHITE : BLACK;
}

PieceType char_to_piece_type(char c)
{
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
        return -1;
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

int pos_list_length(PositionList *pos_l)
{
    if (pos_l == NULL)
    {
        return 0;
    }
    return 1 + pos_list_length(pos_l->tail);
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

bool are_same_pos(BoardState *board_s1, BoardState *board_s2)
{
    Piece(*board1)[8] = board_s1->board;
    Piece(*board2)[8] = board_s2->board;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            if (!are_same_piece(board1[i][j], board2[i][j]))
            {
                return false;
            }
        }
    }
    // clang-format off
    return board_s1->black_kingside_castlable == board_s2->black_kingside_castlable && board_s1->black_queenside_castlable == board_s2->black_queenside_castlable
        && board_s1->white_kingside_castlable == board_s2->white_kingside_castlable && board_s1->white_queenside_castlable == board_s2->white_queenside_castlable
        && board_s1->black_pawn_passant == board_s2->black_pawn_passant && board_s1->white_pawn_passant == board_s2->white_pawn_passant;
    // clang-format on
}

bool threefold_repetition(BoardState *board_s, PositionList *pos_l, int number_of_repetitions)
{
    if (pos_l == NULL)
    {
        return false;
    }
    else
    {
        if (are_same_pos(board_s, pos_l->board_s))
        {
            number_of_repetitions++;
        }
        if (number_of_repetitions > 2)
        {
            return true;
        }
        return threefold_repetition(board_s, pos_l->tail, number_of_repetitions);
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

bool can_move_heuristic(BoardState *board_s, Piece piece, Coords init_co, Coords new_co, bool check_would_stop)
{
    int i = new_co.x;
    int j = new_co.y;
    // clang-format off
    return !(piece.name == ' ')
        && !(piece.name == 'P' && (abs(init_co.x - i) > 2 || abs(init_co.y - j) > 1))
        && !(piece.name == 'N' && abs(init_co.x - i) + abs(init_co.y - j) != 3)
        && !(piece.name == 'B' && abs(init_co.x - i) != abs(init_co.y - j))
        && !(piece.name == 'R' && init_co.x != i && init_co.y != j)
        && !(piece.name == 'Q' && (abs(init_co.x - i) != abs(init_co.y - j) && init_co.x != i && init_co.y != j))
        && !(piece.name == 'K' &&( abs(init_co.x - i) > 1 || abs(init_co.y - j) > 2))
        && can_move(board_s, piece, init_co, new_co, check_would_stop);
    // clang-format on
}

bool is_attacked(BoardState *board_s, Coords co, char color, bool check_would_stop)
{
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Piece piece = board_s->board[i][j];
            if (piece.color != color)
            {
                Coords piece_coords;
                piece_coords.x = i;
                piece_coords.y = j;
                if (can_move_heuristic(board_s, piece, piece_coords, co, check_would_stop))
                {
                    return true;
                }
            }
        }
    }
    return false;
}

BoardState *move_pawn_handling(BoardState *board_s, Piece move_piece, Piece dest_piece, Move sel_move)
{
    Coords new_coords = sel_move.dest_co;
    Coords init_coords = sel_move.init_co;

    // fprintf(stderr, "move_pawn_handling: color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
    // fprintf(stderr, "dest_piece: %c, %c\n", dest_piece.name, dest_piece.color);
    if ((move_piece.color == 'w' && new_coords.x == 7) || (move_piece.color == 'b' && new_coords.x == 0))
    {
        Color color = move_piece.color == 'w' ? WHITE : BLACK;
        board_s->board[new_coords.x][new_coords.y].name = sel_move.promotion;
        board_s->all_pieces_bb[color][char_to_piece_type(sel_move.promotion)] |= 1ULL << coords_to_square(new_coords);
        board_s->all_pieces_bb[color][PAWN] &= ~(1ULL << coords_to_square(new_coords));
        board_s->phase += PIECES_PHASE_VALUES[char_to_piece_type(sel_move.promotion)];
    }
    if (move_piece.color == 'w' && new_coords.x - init_coords.x == 2)
    {
        // fprintf(stderr, "2 pas, color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
        board_s->white_pawn_passant = new_coords.y;
    }
    else if (move_piece.color == 'b' && init_coords.x - new_coords.x == 2)
    {
        board_s->black_pawn_passant = new_coords.y;
    }
    if (move_piece.color == 'w' && is_empty(dest_piece) && new_coords.y != init_coords.y)
    {
        board_s->board[new_coords.x - 1][new_coords.y] = empty_piece();
        board_s->color_bb[BLACK] &= ~(1ULL << (39 - new_coords.y)); // 39 = 4 * 8 + 7 (to get the fith row)
        board_s->all_pieces_bb[BLACK][PAWN] &= ~(1ULL << (39 - new_coords.y));
    }
    else if (move_piece.color == 'b' && is_empty(dest_piece) && new_coords.y != init_coords.y)
    {
        board_s->board[new_coords.x + 1][new_coords.y] = empty_piece();
        board_s->color_bb[WHITE] &= ~(1ULL << (31 - new_coords.y)); // 31 = 3 * 8 + 7 (to get the second row)
        board_s->all_pieces_bb[WHITE][PAWN] &= ~(1ULL << (31 - new_coords.y));
    }
    return board_s;
}

BoardState *move_king_handling(BoardState *board_s, Piece piece, Coords init_coords, Coords new_coords)
{
    Color color = piece.color == 'w' ? WHITE : BLACK;
    if (piece.color == 'w')
    {
        board_s->white_kingside_castlable = false;
        board_s->white_queenside_castlable = false;
    }
    else
    {
        board_s->black_kingside_castlable = false;
        board_s->black_queenside_castlable = false;
    }
    if (new_coords.y == 6 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][5].name = 'R';
        board_s->board[new_coords.x][5].color = piece.color;
        board_s->board[new_coords.x][7] = empty_piece();
        board_s->color_bb[color] ^= 1UL << (8 * new_coords.x);
        board_s->color_bb[color] |= 1UL << (8 * new_coords.x + 2);
        board_s->all_pieces_bb[color][ROOK] ^= 1UL << (8 * new_coords.x);
        board_s->all_pieces_bb[color][ROOK] |= 1UL << (8 * new_coords.x + 2);
    }
    else if (new_coords.y == 2 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][3].name = 'R';
        board_s->board[new_coords.x][3].color = piece.color;
        board_s->board[new_coords.x][0] = empty_piece();
        board_s->color_bb[color] ^= 1UL << (8 * new_coords.x + 7);
        board_s->color_bb[color] |= 1UL << (8 * new_coords.x + 4);
        board_s->all_pieces_bb[color][ROOK] ^= 1UL << (8 * new_coords.x + 7);
        board_s->all_pieces_bb[color][ROOK] |= 1UL << (8 * new_coords.x + 4);
    }
    return board_s;
}

BoardState *move_rook_handling(BoardState *board_s, Piece piece, Coords init_coords, Coords new_coords)
{
    if (piece.color == 'w' && init_coords.x == 0 && init_coords.y == 0)
    {
        board_s->white_queenside_castlable = false;
    }
    else if (piece.color == 'w' && init_coords.x == 0 && init_coords.y == 7)
    {
        board_s->white_kingside_castlable = false;
    }
    else if (piece.color == 'b' && init_coords.x == 7 && init_coords.y == 0)
    {
        board_s->black_queenside_castlable = false;
    }
    else if (piece.color == 'b' && init_coords.x == 7 && init_coords.y == 7)
    {
        board_s->black_kingside_castlable = false;
    }
    return board_s;
}

BoardState *move_piece(BoardState *board_s, Move sel_move)
{
    Coords init_coords = sel_move.init_co;
    Coords new_coords = sel_move.dest_co;
    Piece move_piece = get_piece(board_s->board, init_coords);
    Piece dest_piece = get_piece(board_s->board, new_coords);
    Color color = char_to_color(move_piece.color);
    Color enemy_color = color == WHITE ? BLACK : WHITE;
    if (is_empty(move_piece))
    {
        return board_s;
    }
    // fprintf(stderr, "move_piece: color: %c, init_coords: (%d, %d), new_coords: (%d, %d)\n", move_piece.color, init_coords.x, init_coords.y, new_coords.x, new_coords.y);
    // update board
    board_s->white_pawn_passant = -1;
    board_s->black_pawn_passant = -1;
    // put the piece in the new location
    board_s->board[new_coords.x][new_coords.y].name = move_piece.name;
    board_s->board[new_coords.x][new_coords.y].color = move_piece.color;
    board_s->color_bb[color] |= 1ULL << coords_to_square(new_coords);
    board_s->all_pieces_bb[color][char_to_piece_type(move_piece.name)] |= 1ULL << coords_to_square(new_coords);
    // remove the piece from the old location
    board_s->board[init_coords.x][init_coords.y] = empty_piece();
    board_s->color_bb[color] ^= 1ULL << coords_to_square(init_coords);
    board_s->all_pieces_bb[color][char_to_piece_type(move_piece.name)] ^= 1ULL << coords_to_square(init_coords);
    // remove the piece from the enemy if it exists
    if (dest_piece.color != ' ')
    {
        board_s->color_bb[enemy_color] ^= 1ULL << coords_to_square(new_coords);
        board_s->all_pieces_bb[enemy_color][char_to_piece_type(dest_piece.name)] ^= 1ULL << coords_to_square(new_coords);
        board_s->phase -= PIECES_PHASE_VALUES[char_to_piece_type(dest_piece.name)];
    }
    // fifty move rule
    if (dest_piece.name == ' ' && move_piece.name != 'P')
    {
        board_s->fifty_move_rule++;
    }
    else
    {
        board_s->fifty_move_rule = 0;
    }
    // handle special moves
    if (move_piece.name == 'P')
    {
        board_s = move_pawn_handling(board_s, move_piece, dest_piece, sel_move);
    }
    else if (move_piece.name == 'K')
    {
        board_s = move_king_handling(board_s, move_piece, init_coords, new_coords);
    }
    else if (move_piece.name == 'R')
    {
        board_s = move_rook_handling(board_s, move_piece, init_coords, new_coords);
    }
    // switch player
    board_s->player = board_s->player == WHITE ? BLACK : WHITE;
    return board_s;
}

BoardState *move_piece_forced(BoardState *board_s, Move cur_move)
{
    Coords init_coords = cur_move.init_co;
    Coords new_coords = cur_move.dest_co;
    if (is_empty_coords(init_coords) || is_empty_coords(new_coords))
    {
        return board_s;
    }
    if (init_coords.x < 0 || init_coords.x > 7 || init_coords.y < 0 || init_coords.y > 7 || new_coords.x < 0 || new_coords.x > 7 || new_coords.y < 0 || new_coords.y > 7)
    {
        return board_s;
    }
    Piece move_piece = get_piece(board_s->board, init_coords);
    //  castling
    if (move_piece.name == 'K' && new_coords.y == 6 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][5].name = 'R';
        board_s->board[new_coords.x][5].color = move_piece.color;
        board_s->board[new_coords.x][7] = empty_piece();
    }
    else if (move_piece.name == 'K' && new_coords.y == 2 && init_coords.y == 4)
    {
        board_s->board[new_coords.x][3].name = 'R';
        board_s->board[new_coords.x][3].color = move_piece.color;
        board_s->board[new_coords.x][0] = empty_piece();
    }
    // en passant
    if (move_piece.name == 'P' && is_empty(board_s->board[new_coords.x][new_coords.y]) && new_coords.y != init_coords.y && ((init_coords.x == 3 && new_coords.x == 2) || (init_coords.x == 4 && new_coords.x == 5)))
    {
        if (move_piece.color == 'w')
        {
            board_s->board[new_coords.x + 1][new_coords.y] = empty_piece();
        }
        else
        {
            board_s->board[new_coords.x - 1][new_coords.y] = empty_piece();
        }
    }
    // normal move
    board_s->board[new_coords.x][new_coords.y].name = move_piece.name;
    // promotion
    if (((move_piece.color == 'w' && new_coords.x == 7) || (move_piece.color == 'b' && new_coords.x == 0)) && cur_move.promotion != ' ')
    {
        board_s->board[new_coords.x][new_coords.y].name = cur_move.promotion;
    }
    // finish
    board_s->board[new_coords.x][new_coords.y].color = move_piece.color;
    board_s->board[init_coords.x][init_coords.y] = empty_piece();
    return board_s;
}

Coords find_king(BoardState *board_s, char color)
{
    Piece(*board)[8] = board_s->board;
    Coords king_coords = empty_coords();
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            Piece piece = board[i][j];
            if (piece.name == 'K' && piece.color == color)
            {
                king_coords.x = i;
                king_coords.y = j;
                return king_coords;
            }
        }
    }
    return king_coords;
}

bool is_check(BoardState *board_s, char color)
{
    Coords king_coords = find_king(board_s, color);
    return is_attacked(board_s, king_coords, color, false);
}

bool is_mate(BoardState *board_s, char color)
{
    Piece(*board)[8] = board_s->board;
    Coords init_coords;
    Coords new_coords;
    Piece piece;
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 8; j++)
        {
            piece = board[i][j];
            if (piece.color == color)
            {
                init_coords.x = i;
                init_coords.y = j;
                for (int k = 0; k < 8; k++)
                {
                    for (int l = 0; l < 8; l++)
                    {
                        new_coords.x = k;
                        new_coords.y = l;
                        if (can_move_heuristic(board_s, piece, init_coords, new_coords, true))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool is_checkmate(BoardState *board_s, char color)
{
    if (!is_check(board_s, color))
    {
        return false;
    }
    return is_mate(board_s, color);
}

bool is_stalemate(BoardState *board_s, char color)
{
    if (is_check(board_s, color))
    {
        return false;
    }
    return is_mate(board_s, color);
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
            board_s->board[i][j].name = ' ';
            board_s->board[i][j].color = ' ';
        }
    }
    for (int i = 0; i < 8; i++)
    {
        board_s->board[1][i].name = 'P';
        board_s->board[6][i].name = 'P';
        board_s->board[0][i].color = 'w';
        board_s->board[1][i].color = 'w';
        board_s->board[6][i].color = 'b';
        board_s->board[7][i].color = 'b';
    }
    for (int i = 0; i < 8; i = i + 7)
    {
        board_s->board[i][0].name = 'R';
        board_s->board[i][7].name = 'R';
        board_s->board[i][1].name = 'N';
        board_s->board[i][6].name = 'N';
        board_s->board[i][2].name = 'B';
        board_s->board[i][5].name = 'B';
        board_s->board[i][3].name = 'Q';
        board_s->board[i][4].name = 'K';
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

    return board_s;
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
                board_s->board[7 - xx][y].name = ' ';
                board_s->board[7 - xx][y].color = ' ';
                y++;
            }
        }
        else
        {
            if (FEN[i] >= 'A' && FEN[i] <= 'Z')
            {
                board_s->board[7 - xx][y].color = 'w';
                board_s->board[7 - xx][y].name = FEN[i];
                board_s->color_bb[WHITE] |= 1ULL << (8 * (7 - xx) + 7 - y);
                board_s->all_pieces_bb[WHITE][char_to_piece_type(FEN[i])] |= 1ULL << (8 * (7 - xx) + 7 - y);
            }
            else
            {
                board_s->board[7 - xx][y].color = 'b';
                board_s->board[7 - xx][y].name = FEN[i] - 'a' + 'A';
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
    return board_s;
}

bool can_move(BoardState *board_s, Piece piece, Coords init_co, Coords new_co, bool check_would_stop)
{
    // pre-checks to see if the move is valid
    if (new_co.x < 0 || new_co.x >= 8 || new_co.y < 0 || new_co.y >= 8)
    {
        return false;
    }
    if (get_piece(board_s->board, new_co).color == piece.color)
    {
        return false;
    }
    if (check_would_stop)
    {
        BoardState *new_board_s = malloc(sizeof(BoardState));
        if (new_board_s == NULL)
        {
            return false;
        }
        Move sel_move;
        sel_move.init_co = init_co;
        sel_move.dest_co = new_co;
        sel_move.promotion = ' ';
        *new_board_s = *board_s;
        // fprintf(stderr, "can_move: %c (%d, %d) -> (%d, %d)\n", piece.name, init_co.x, init_co.y, new_co.x, new_co.y);
        new_board_s = move_piece_forced(new_board_s, sel_move);
        if (is_check(new_board_s, piece.color))
        {
            free(new_board_s);
            return false;
        }
        free(new_board_s);
    }

    // fprintf(stderr, "piece is %c of color %c\n", piece.name, piece.color);
    switch (piece.name)
    {
    case 'P':
        return can_move_pawn(board_s, piece, init_co, new_co);
    case 'R':
        return can_move_rook(board_s, init_co, new_co);
    case 'N':
        return can_move_knight(board_s, init_co, new_co);
    case 'B':
        return can_move_bishop(board_s, init_co, new_co);
    case 'Q':
        return can_move_queen(board_s, init_co, new_co);
    case 'K':
        return can_move_king(board_s, piece, init_co, new_co);
    default:
        return false;
    }
}