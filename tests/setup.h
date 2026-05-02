#ifndef __SCE_UNITTEST_SETUP_H__
#define __SCE_UNITTEST_SETUP_H__

#include "../include/dev.h"

#define BOARD_CLEAR_SETUP() \
    SCE_Context ctx; \
    SCE_Precomputation_Tables precomputation_tables; \
    SCE_Precomputation_Tables_init(&precomputation_tables, NULL); \
    SCE_Chessboard& board { ctx.board }; \
    SCE_Context_init(&ctx, &precomputation_tables); \
    SCE_Chessboard_clear(&ctx); \
    ctx.board.zobrist_hash = SCE_Chessboard_ComputeZobristHash(&ctx);



#define BOARD_SETUP() \
    SCE_Context ctx; \
    SCE_Precomputation_Tables precomputation_tables; \
    SCE_Precomputation_Tables_init(&precomputation_tables, NULL); \
    SCE_Context_init(&ctx, &precomputation_tables); \
    SCE_Chessboard& board { ctx.board };

#define MOVE_LIST_SETUP(list, n_moves) \
    SCE_ChessMoveList list; \
    list.count = 0; \
    ASSERT_EQ(SCE_GeneratePseudoLegalMoves(&list, &ctx, false), SCE_SUCCESS); \
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
