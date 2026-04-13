#ifdef DEBUG

#ifndef SCE_DEV_H
#define SCE_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "chess.h"

typedef unsigned int uint;

void print_as_board(const uint64_t val);

// Returns 1 for success, 0 for failure
SCE_Return place_piece_on_board(SCE_Chessboard* const ptr_board, const char * const an, uint piece_type);

SCE_Return print_move_to_AN(const SCE_ChessMove move);

SCE_Return debug_print_board(const SCE_Chessboard* const ptr_board);

// Returns number of PERFT count from the current board. 0 for failure.
unsigned long long perft_count(const SCE_Chessboard* const ptr_board, const SCE_PieceMovementPrecomputationTable* const ptr_precomputation_table, const uint depth, const bool root);

#ifdef __cplusplus
}
#endif

#endif  // SCE_DEV_H
#endif  // DEBUG
