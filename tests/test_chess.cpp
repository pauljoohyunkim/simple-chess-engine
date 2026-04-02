#include <gtest/gtest.h>
#include "../include/chess.h"

TEST(ChessBoard, Check_Under_Attack_1) {
    SCE_PieceMovementPrecomputationTable precpt_tbl;
    SCE_PieceMovementPrecompute(&precpt_tbl);
    SCE_Chessboard board;
    SCE_Chessboard_reset(&board);
    SCE_IsSqaureAttacked(&board, &precpt_tbl, 1, WHITE);
}
