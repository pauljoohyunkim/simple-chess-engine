#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "../include/fen.h"
#include "setup.h"

// https://analog-hors.github.io/webperft/ for debugging

typedef unsigned long long ull;

TEST(PERFT, Initial_Depth_1_to_5) {
    const ull testvector[5U] = { 20, 400, 8902, 197281, 4865609 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        BOARD_SETUP()
        (void)board;

        const ull count = perft_count(&ctx, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }

}

#ifndef UNITTEST_FULL
TEST(PERFT, Kiwipete_Depth_1_to_4) {
#else
TEST(PERFT, Kiwipete_Depth_1_to_6) {
#endif

#ifndef UNITTEST_FULL
    const uint testvector[] = { 48, 2039, 97862, 4085603 };
#else
    const ull testvector[] = { 48, 2039, 97862, 4085603, 193690690, 8031647685 };
#endif
    for (uint depth = 1U; depth <= sizeof(testvector)/sizeof(testvector[0]); depth++) {
        SCE_Context ctx;
        SCE_Chessboard& board { ctx.board };
        SCE_Precomputation_Tables precomputation_tables;
        ASSERT_EQ(SCE_Precomputation_Tables_init(&precomputation_tables, NULL), SCE_SUCCESS);
        ASSERT_EQ(SCE_Context_init(&ctx, &precomputation_tables), SCE_SUCCESS);
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);

        board.zobrist_hash = SCE_Chessboard_ComputeZobristHash(&ctx);

        // debug_print_board(&ctx);

        const ull count = perft_count(&ctx, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}

TEST(PERFT, Position3) {
    const ull testvector[4U] = { 14, 191, 2812, 43238 };
    for (uint depth = 1U; depth <= 4U; depth++) {
        SCE_Context ctx;
        SCE_Chessboard& board { ctx.board };
        SCE_Precomputation_Tables precomputation_tables;
        ASSERT_EQ(SCE_Precomputation_Tables_init(&precomputation_tables, NULL), SCE_SUCCESS);
        ASSERT_EQ(SCE_Context_init(&ctx, &precomputation_tables), SCE_SUCCESS);
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), SCE_SUCCESS);

        board.zobrist_hash = SCE_Chessboard_ComputeZobristHash(&ctx);

        // debug_print_board(&ctx);

        const ull count = perft_count(&ctx, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}
