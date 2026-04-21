#include <gtest/gtest.h>
#include "chess.h"
#include "dev.h"
#include "fen.h"

// https://www.chess.com/terms/fen-chess
TEST(FEN, RandomFEN) {
    SCE_Context ctx;
    SCE_Chessboard& board { ctx.board };
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&ctx, "8/5k2/3p4/1p1Pp2p/pP2Pp1P/P4P1K/8/8 b - - 99 50"), SCE_SUCCESS);

    ASSERT_EQ(board.to_move, BLACK);
    ASSERT_EQ(board.castling_rights, 0U);
    ASSERT_EQ(board.en_passant_idx, UNASSIGNED);

    const uint64_t occupancy_w = SCE_Chessboard_Occupancy_Color(&ctx, WHITE);
    const uint64_t occupancy_b = SCE_Chessboard_Occupancy_Color(&ctx, BLACK);
    ASSERT_TRUE(occupancy_b);
    ASSERT_TRUE(occupancy_w);

    debug_print_board(&ctx);
}
