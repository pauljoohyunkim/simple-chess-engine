#include <gtest/gtest.h>
#include "../include/chess.h"

#define BOARD_SETUP(board, precpt_tbl) \
    SCE_PieceMovementPrecomputationTable precpt_tbl; \
    SCE_PieceMovementPrecompute(&precpt_tbl); \
    SCE_Chessboard board; \
    SCE_Chessboard_reset(&board);


TEST(ChessBoard, Check_Under_Attack_1) {
    BOARD_SETUP(board, precpt_tbl);
    ASSERT_TRUE(SCE_IsSqaureAttacked(&board, &precpt_tbl, 1, WHITE));
}
