#include <gtest/gtest.h>
#include "../include/chess.h"
#include "../include/dev.h"
#include "../include/fen.h"
#include "setup.h"

// https://analog-hors.github.io/webperft/ for debugging

TEST(PERFT, Initial_Depth_1_to_5) {
    const uint testvector[5U] = { 20, 400, 8902, 197281, 4865609 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        BOARD_SETUP(board, precomputation_table)

        const uint count = perft_count(&board, &precomputation_table, depth);

        ASSERT_EQ(count, testvector[depth-1]);
    }

}

// Kiwipete Debugging
/*
a2a3: 44
b2b3: 42
g2g3: 42
d5d6: 41
a2a4: 44
g2g4: 42
g2h3: 43
d5e6: 46
c3b1: 42
c3d1: 42
c3a4: 42
c3b5: 39
e5d3: 43
e5c4: 42
e5g4: 44
e5c6: 41
e5g6: 42
e5d7: 45
e5f7: 44
d2c1: 43
d2e3: 43
d2f4: 43
d2g5: 42
d2h6: 41
e2d1: 44
e2f1: 44
e2d3: 42
e2c4: 41
e2b5: 39
e2a6: 36
a1b1: 43
a1c1: 43
a1d1: 43
h1f1: 43
h1g1: 43
f3d3: 42
f3e3: 43
f3g3: 43
f3h3: 43
f3f4: 43
f3g4: 43
f3f5: 45
f3h5: 43
f3f6: 39
e1d1: 43
e1f1: 43
e1g1: 43
e1c1: 43
*/
TEST(PERFT, Kiwipete_Depth_2) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        SCE_ChessMove move = (SCE_AN_To_Idx("A2") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("A3") SCE_CHESSMOVE_SET_DST);
        SCE_Return ret = SCE_MakeMove(&board, &precpt_tbl, move);
        ASSERT_EQ(ret, SCE_SUCCESS);

        SCE_ChessMoveList pseudolegal_moves;
        SCE_ChessMoveList_clear(&pseudolegal_moves);
        debug_print_board(&board);
        ret = SCE_GeneratePseudoLegalMoves(&pseudolegal_moves, &board, &precpt_tbl);
        for (uint i = 0U; i < pseudolegal_moves.count; i++) {
            ret = SCE_MakeMove(&board, &precpt_tbl, pseudolegal_moves.moves[i]);
            if (ret == SCE_SUCCESS) {
                print_move_to_AN(pseudolegal_moves.moves[i]);
                ret = SCE_UnmakeMove(&board, &precpt_tbl);
            }
        }


        const uint count = perft_count(&board, &precpt_tbl, 1);

        ASSERT_EQ(count, 44);
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

/*
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
*/