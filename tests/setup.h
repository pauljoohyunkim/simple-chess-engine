#ifndef __SCE_UNITTEST_SETUP_H__
#define __SCE_UNITTEST_SETUP_H__

#include "../include/dev.h"

#define BOARD_CLEAR_SETUP(board) \
    SCE_Chessboard board; \
    SCE_Chessboard_clear(&board);


#define BOARD_SETUP(board, precpt_tbl, zobrist_table) \
    SCE_PieceMovementPrecomputationTable precpt_tbl; \
    SCE_PieceMovementPrecompute(&precpt_tbl); \
    SCE_Chessboard board; \
    SCE_Chessboard_reset(&board); \
    SCE_ZobristTable zobrist_table; \
    SCE_ZobristTable_init(&zobrist_table, NULL);

#define MOVE_LIST_SETUP(list, n_moves) \
    SCE_ChessMoveList list; \
    list.count = 0; \
    ASSERT_EQ(SCE_GeneratePseudoLegalMoves(&list, &board, &precpt_tbl), SCE_SUCCESS); \
    uint n_moves[N_TYPES_PIECES] = { 0 }; \
    for (unsigned int i = 0; i < list.count; i++) { \
        print_move_to_AN(list.moves[i]); \
        uint64_t src = 1ULL << (list.moves[i] SCE_CHESSMOVE_GET_SRC); \
        for (uint piece_type = W_PAWN; piece_type <= B_KING; piece_type++) { \
            if (src & board.bitboards[piece_type]) { \
                n_moves[piece_type]++; \
            } \
        } \
    }
#endif  // __SCE_UNITTEST_SETUP_H__
