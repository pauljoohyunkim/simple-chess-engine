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

        const uint count = perft_count(&board, &precomputation_table, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }

}

// Kiwipete + a1d1
TEST(PERFT, Custom_PERFT) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/2R1K2R b Kkq - 1 1"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        SCE_Return ret;

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


        const uint count = perft_count(&board, &precpt_tbl, 2, false);

        ASSERT_EQ(count, 1968);
}


TEST(PERFT, Kiwipete_Depth_1_to_5) {
    const uint testvector[5U] = { 48, 2039, 97862, 4085603, 193690690 };
    for (uint depth = 1U; depth <= 5U; depth++) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        debug_print_board(&board);

        const uint count = perft_count(&board, &precpt_tbl, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}

TEST(PERFT, Position3) {
    const uint testvector[4U] = { 14, 191, 2812, 43238 };
    for (uint depth = 1U; depth <= 4U; depth++) {
        SCE_Chessboard board;
        ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), SCE_SUCCESS);
        SCE_PieceMovementPrecomputationTable precpt_tbl;
        SCE_PieceMovementPrecompute(&precpt_tbl);

        debug_print_board(&board);

        const uint count = perft_count(&board, &precpt_tbl, depth, false);

        ASSERT_EQ(count, testvector[depth-1]);
    }
}

/*
// Position 3 + e2e4 + f4f3 + {a5a4, a5a6, b4a4, b4b1, b4b2, b4b3, b4c4, g2g4}
// Position 3 + g2g4 + ...
// Position 3 + b4b* + ...
TEST(PERFT, Position3_Debug) {
    SCE_Chessboard board;
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"), SCE_SUCCESS);
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);

    SCE_ChessMove move = (SCE_AN_To_Idx("E2") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("E4") SCE_CHESSMOVE_SET_DST) | (SCE_CHESSMOVE_FLAG_DOUBLE_PAWN_PUSH SCE_CHESSMOVE_SET_FLAG);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    move = (SCE_AN_To_Idx("F4") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("F3") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    move = (SCE_AN_To_Idx("A5") SCE_CHESSMOVE_SET_SRC) | (SCE_AN_To_Idx("A4") SCE_CHESSMOVE_SET_DST);
    ASSERT_EQ(SCE_MakeMove(&board, &precpt_tbl, move), SCE_SUCCESS);

    SCE_Return ret = debug_print_board(&board);

    const uint count = perft_count(&board, &precpt_tbl, 1, true);

    ASSERT_EQ(count, 17);
}
*/
