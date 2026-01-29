#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "types.h"
#include "chess_logic.h"

// create magic!
// https://analog-hors.github.io/site/magic-bitboards/

typedef uint64_t Bitboard;

Bitboard zobrist_table[781];

int min(int a, int b)
{
    if (a < b)
        return a;
    else
        return b;
}

void print_bitboard(Bitboard b)
{
    for (int i = 63; i >= 0; i--)
    {
        printf("%d", (int)(b >> i) & 1);
        if (i % 8 == 0)
        {
            printf("\n");
        }
    }
    printf("\n");
}

Bitboard random_bitboard()
{
    Bitboard res = 0;
    for (int i = 0; i < 64; i++)
    {
        if (rand() % 2 == 0)
        {
            res |= 1ULL << i;
        }
    }
    return res;
}

void save_int_array_to_file(const int *array, size_t size, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    fprintf(file, "{");
    for (size_t i = 0; i < size; ++i)
    {
        fprintf(file, "%d", array[i]);
        if (i < size - 1)
        {
            fprintf(file, ", ");
        }
    }
    fprintf(file, "}");

    fclose(file);
}

void save_bb_array_to_file(const Bitboard *array, size_t size, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        perror("Erreur lors de l'ouverture du fichier");
        return;
    }

    fprintf(file, "{");
    for (size_t i = 0; i < size; ++i)
    {
        fprintf(file, "%luULL", array[i]);
        if (i < size - 1)
        {
            fprintf(file, ", ");
        }
    }
    fprintf(file, "}");

    fclose(file);
}

void generate_zobrist()
{
    int index = 0;
    // pieces
    for (int color = 0; color < 2; color++)
    {
        for (int piece_type = 0; piece_type < 6; piece_type++)
        {
            for (int piece_color = WHITE; piece_color <= BLACK; piece_color++)
            {
                for (int x = 0; x < 8; x++)
                {
                    for (int y = 0; y < 8; y++)
                    {
                        zobrist_table[index] = random_bitboard();
                        index++;
                    }
                }
            }
        }
    }
    // castling rights
    for (int i = 0; i < 16; i++)
    {
        zobrist_table[index] = random_bitboard();
        index++;
    }
    // en passant files
    for (int i = 0; i < 8; i++)
    {
        zobrist_table[index] = random_bitboard();
        index++;
    }
    // side to move
    zobrist_table[index] = random_bitboard();
}

int main()
{
    generate_zobrist();

    save_bb_array_to_file(zobrist_table, 781, "builds/magik/zobrist.txt");
    return 0;
}