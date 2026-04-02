#include <stdio.h>
#include "dev.h"
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val) {
    printf("\n");
    for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
        for (uint j = 0; j < CHESSBOARD_DIMENSION; j++) {
            uint64_t pos = 1ULL << ((7-i) * CHESSBOARD_DIMENSION + (7-j));
            if (pos & val) {
                printf("*");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
    printf("\n");
}

int place_piece_on_board(SCE_Chessboard* const ptr_board, const char * const an, uint piece_type) {
    if (ptr_board == NULL || an == NULL || piece_type >= N_TYPES_PIECES) return SCE_FAILURE;

    // Get bitboard form of algebraic notation
    const uint64_t loc = SCE_AN_To_Bitboard(an);
    if (loc == 0) return SCE_FAILURE;

    // Empty out the specific loc.
    for (uint i = 0; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] &= ~loc;
    }

    ptr_board->bitboards[piece_type] ^= loc;

    return SCE_SUCCESS;
}
