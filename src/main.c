#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "types.h"
#include "transposition_tables.h"
#include "alphabeta.h"
#include "chess_logic.h"
#include "interface_uci_like.h"
#include "debug_functions.h"
#include "eval.h"
#include "bitboards_moves.h"
#define MAX_MSG_LENGTH 32000
#define TT_SIZE (1 << 22) // Entries in transposition table

void print_board(BoardState *board_s)
{
    Piece(*board)[8] = board_s->board;
    for (int i = 7; i >= 0; i--)
    {
        printf("%d ", i + 1);
        for (int j = 0; j < 8; j++)
        {
            if (board[i][j].color == WHITE)
            {
                printf("%c ", board[i][j].name);
            }
            else if (board[i][j].color == BLACK)
            {
                printf("%c ", board[i][j].name + 32);
            }
            else
            {
                printf("  ");
            }
        }
        printf("\n");
    }
    printf("  a b c d e f g h\n");
}

void test_self_engine(double time_white, double time_black)
{    
    TranspoTable global_transpo_table;
    initialize_transposition_table(&global_transpo_table, TT_SIZE);

    PositionList *board_history = empty_list();
    BoardState *board_s = init_board();
    board_history = save_position(board_s, board_history);
    Move move = empty_move();
    Color color = WHITE;
    print_board(board_s);

    for (int i = 0; i < 1000; i++)
    {
        /*
        // ask for ENTER key
        printf("Press [Enter] key to continue.\n");
        fflush(stdin); // option ONE to clean stdin
        getchar();     // wait for ENTER
        */
        if (color == WHITE)
        {
            move = iterative_deepening(&global_transpo_table, board_history, color, 20, time_white);
        }
        else
        {
            move = iterative_deepening(&global_transpo_table, board_history, color, 20, time_black);
        }
        board_s = move_piece(board_s, move);
        board_history = save_position(board_s, board_history);

        // change color
        color ^= 1;
        print_board(board_s);
        print_move(move);
        if (is_king_in_check(board_s))
        {
            printf("check\n");
        }
        if (is_mate_bb(board_s))
        {
            if (is_king_in_check(board_s))
            {
                printf("checkmate\n");
            }
            else
            {
                printf("stalemate\n");
            }
            break;
        }
        if (threefold_hash(board_s->hash, board_history, 0))
        {
            printf("draw by threefold repetition\n");
            break;
        }
        if (board_s->fifty_move_rule >= 100)
        {
            printf("draw by fifty move rule\n");
            break;
        }
    }
    free(board_s);
    free_position_list(board_history);
}

void test_uci_solo()
{
    char buffer[MAX_MSG_LENGTH] = {0};

    TranspoTable global_transpo_table;
    initialize_transposition_table(&global_transpo_table, TT_SIZE);

    PositionList *board_history = malloc(sizeof(PositionList));
    if (board_history == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    board_history->tail = NULL;
    board_history->board_s = NULL;

    const char *commands[] = {
        "position fen \"rnbqkbnr/p3pppp/2p5/3p4/8/2N5/PPPPPPPP/R1BQKBNR w KQkq - 0 1\" moves e2e4 e7e5 g1f3\n",
        "go wtime 1000 btime 1000\n",
        "position fen \"r1b2rk1/ppq1ppbp/2n2np1/2ppN3/5P2/1P2P3/PBPPB1PP/RN1Q1RK1 w - -\" moves e5c6 b7c6 d2d4 c5d4 b2d4 c6c5 d4f6 e7f6 d1d5\n",
        "go wtime 1000 btime 1000\n",
        "position startpos moves e2e4\n",
        "go wtime 1000 btime 1000\n",
        "position startpos moves e2e4 d7d5\n",
        "go wtime 1000 btime 0\n",
        "position startpos moves e2e4 d7d5 f1b5\n",
        "go depth 4 wtime 0 btime 10000\n",
        "position startpos moves e2e4 d7d5 f1b5\n",
        "go wtime 0 btime infinite depth 5\n",
        "quit\n"};

    int i = 0;
    while (strcmp(buffer, "quit\n") != 0)
    {
        strcpy(buffer, commands[i]);
        fprintf(stderr, "Debug: Received message from main program:\n %s\n\n", buffer);
        handle_uci_command(buffer, &global_transpo_table, board_history);
        i++;
    }
    free_position_list(board_history);
}

void answer_uci()
{
    char buffer[MAX_MSG_LENGTH] = {0};

    TranspoTable global_transpo_table;
    initialize_transposition_table(&global_transpo_table, TT_SIZE);

    PositionList *board_history = malloc(sizeof(PositionList));
    if (board_history == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    board_history->tail = NULL;
    board_history->board_s = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL)
    {
        // Check if the input was too long
        if (strlen(buffer) == sizeof(buffer) - 1 && buffer[sizeof(buffer) - 2] != '\n')
        {
            fprintf(stderr, "Error: Input exceeds maximum length of 1023 characters.\n");
            exit(EXIT_FAILURE);
            // while (fgetc(stdin) != '\n');
            // continue;
        }
        fprintf(stderr, "Debug: Received message length from main program: %ld\n", strlen(buffer));
        fprintf(stderr, "Debug: Received message from main program: %s\n", buffer);
        handle_uci_command(buffer, &global_transpo_table, board_history);
        if (strcmp(buffer, "quit\n") == 0)
        {
            break;
        }
    }
    fprintf(stderr, "Debug: 2\n");
    free_position_list(board_history);
}

int main()
{
    // test_self_engine(1.0, 1.0);
    // test_uci_solo();
    init_eval_tables();
    answer_uci(); // C'est dans cette fonction que tout est initialisé
    return 0;
}