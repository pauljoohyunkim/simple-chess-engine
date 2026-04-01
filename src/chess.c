#include <stdlib.h>
#include "chess.h"

#define RETURN_IF_FAILURE(x) do { if (x == SCE_FAILURE) return SCE_FAILURE; } while (0);

typedef unsigned int uint;

int SCE_Chessboard_clear(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    for (uint i = 0U; i < N_TYPES_PIECES; i++) {
        ptr_board->bitboards[i] = 0U;
    }

    return SCE_SUCCESS;
}

int SCE_Chessboard_reset(SCE_Chessboard* const ptr_board) {
    if (ptr_board == NULL) return SCE_FAILURE;

    RETURN_IF_FAILURE(SCE_Chessboard_clear(ptr_board));

    // White pieces
    ptr_board->bitboards[W_PAWN] = PAWN_INITIAL_ROW << (8U * 1U);
    ptr_board->bitboards[W_KNIGHT] = KNIGHT_INITIAL_ROW;
    ptr_board->bitboards[W_BISHOP] = BISHOP_INITIAL_ROW;
    ptr_board->bitboards[W_ROOK] = ROOK_INITIAL_ROW;
    ptr_board->bitboards[W_QUEEN] = QUEEN_INITIAL_ROW;
    ptr_board->bitboards[W_KING] = KING_INITIAL_ROW;

    // Black pieces
    ptr_board->bitboards[B_PAWN] = PAWN_INITIAL_ROW << (8U * 6U);
    ptr_board->bitboards[B_KNIGHT] = KNIGHT_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_BISHOP] = BISHOP_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_ROOK] = ROOK_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_QUEEN] = QUEEN_INITIAL_ROW << (8U * 7U);
    ptr_board->bitboards[B_KING] = KING_INITIAL_ROW << (8U * 7U);

    return SCE_SUCCESS;
}
