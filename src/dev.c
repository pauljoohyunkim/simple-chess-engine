#include <stdio.h>
#include "dev.h"
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val) {
    printf("\n");
    for (uint i = 0; i < CHESSBOARD_DIMENSION; i++) {
        for (uint j = 0; j < CHESSBOARD_DIMENSION; j++) {
            uint64_t pos = 1ULL << ((7-i) * CHESSBOARD_DIMENSION + j);
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

int print_move_to_AN(const SCE_ChessMove move) {
    const uint64_t src = 1ULL << (move SCE_CHESSMOVE_GET_SRC);
    const uint64_t dst = 1ULL << (move SCE_CHESSMOVE_GET_DST);
    const uint flag = move SCE_CHESSMOVE_GET_FLAG;

    char an_src[3U] = { 0 };
    char an_dst[3U] = { 0 };
    //int ret = SCE_SUCCESS;

    SCE_Bitboard_To_AN(an_src, src);
    SCE_Bitboard_To_AN(an_dst, dst);

    printf("%s -> %s ", an_src, an_dst);
    if (flag & SCE_CHESSMOVE_FLAG_CAPTURE) {
        printf("CAPTURE ");
    }
    if ((flag & 0x7U) == SCE_CHESSMOVE_FLAG_PROMOTE_TO_KNIGHT) {
        printf("PROMOTE TO KNIGHT ");
    }
    if ((flag & 0x7U) == SCE_CHESSMOVE_FLAG_PROMOTE_TO_BISHOP) {
        printf("PROMOTE TO BISHOP ");
    }
    if ((flag & 0x7U) == SCE_CHESSMOVE_FLAG_PROMOTE_TO_ROOK) {
        printf("PROMOTE TO ROOK ");
    }
    if ((flag & 0x7U) == SCE_CHESSMOVE_FLAG_PROMOTE_TO_QUEEN) {
        printf("PROMOTE TO QUEEN ");
    }
    if ((flag & 0x7U) == SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH) {
        printf("PAWN DOUBLE PUSH ");
    }

    printf("\n");

    return SCE_SUCCESS;
}
