#include <gtest/gtest.h>
#include "chess.h"
#include "dev.h"

TEST(FEN, KingsOnly) {
    SCE_Chessboard board;
    ASSERT_EQ(SCE_Chessboard_FEN_setup(&board, "5k2/8/8/8/8/8/8/5K2"), SCE_SUCCESS);

    debug_print_board(&board);
}
