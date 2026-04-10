#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "../include/fen.h"
#include "setup.h"

TEST(PERFT, Initial_Depth_1_to_5) {
    const uint testvector[5U] = { 20, 400, 8902, 197281, 4865609 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        BOARD_SETUP(board, precomputation_table)

        const uint count = perft_count(&board, &precomputation_table, depth);

        ASSERT_EQ(count, testvector[depth-1]);
    }

}


TEST(PERFT, Kiwipete_Depth_1_to_5) {
    const uint testvector[5U] = { 48, 2039, 97862, 4085603, 193690690 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        debug_print_board(&board);

        const uint count = perft_count(&board, &precpt_tbl, depth);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}

TEST(PERFT, TEST3) {
    const uint testvector[4U] = { 14, 191, 2812, 43238 };
    for (uint depth = 1U; depth <= 4U; depth++) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        debug_print_board(&board);

        const uint count = perft_count(&board, &precpt_tbl, depth);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}
